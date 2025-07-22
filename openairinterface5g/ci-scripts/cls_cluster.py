#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#---------------------------------------------------------------------
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import logging
import re
import time
import os

import cls_oai_html
import cls_analysis
import constants as CONST
import helpreadme as HELP
import cls_containerize
import cls_cmd

IMAGE_REGISTRY_SERVICE_NAME = "image-registry.openshift-image-registry.svc"
NAMESPACE = "oaicicd-ran"
OCUrl = "https://api.oai.cs.eurecom.fr:6443"
OCRegistry = "default-route-openshift-image-registry.apps.oai.cs.eurecom.fr"
CI_OC_RAN_NAMESPACE = "oaicicd-ran"

def OC_login(cmd, ocUserName, ocPassword, ocProjectName):
	if ocUserName == '' or ocPassword == '' or ocProjectName == '':
		HELP.GenericHelp(CONST.Version)
		raise ValueError('Insufficient Parameter: no OC Credentials')
	if OCRegistry.startswith("http") or OCRegistry.endswith("/"):
		raise ValueError(f'ocRegistry {OCRegistry} should not start with http:// or https:// and not end on a slash /')
	ret = cmd.run(f'oc login -u {ocUserName} -p {ocPassword} --server {OCUrl}')
	if ret.returncode != 0:
		logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
		return False
	ret = cmd.run(f'oc project {ocProjectName}')
	if ret.returncode != 0:
		logging.error(f'\u001B[1mUnable to access OC project {ocProjectName}\u001B[0m')
		OC_logout(cmd)
		return False
	return True

def OC_logout(cmd):
	cmd.run(f'oc logout')

class Cluster:
	def __init__(self):
		self.eNBIPAddress = ""
		self.eNBSourceCodePath = ""
		self.forcedWorkspaceCleanup = False
		self.OCUserName = ""
		self.OCPassword = ""
		self.OCProjectName = ""
		self.OCUrl = OCUrl
		self.OCRegistry = OCRegistry
		self.ranRepository = ""
		self.ranBranch = ""
		self.ranCommitID = ""
		self.ranAllowMerge = False
		self.ranTargetBranch = ""
		self.cmd = None

	def _recreate_entitlements(self):
		# recreating entitlements, don't care if deletion fails
		self.cmd.run(f'oc delete secret etc-pki-entitlement')
		ret = self.cmd.run(f"oc get secret etc-pki-entitlement -n openshift-config-managed -o json | jq 'del(.metadata.resourceVersion)' | jq 'del(.metadata.creationTimestamp)' | jq 'del(.metadata.uid)' | jq 'del(.metadata.namespace)' | oc create -f -", silent=True)
		if ret.returncode != 0:
			logging.error("could not create secret/etc-pki-entitlement")
			return False
		return True

	def _recreate_bc(self, name, newTag, filename):
		self._retag_image_statement(name, name, newTag, filename)
		self.cmd.run(f'oc delete -f {filename}')
		ret = self.cmd.run(f'oc create -f {filename}')
		if re.search(r'buildconfig.build.openshift.io/[a-zA-Z\-0-9]+ created', ret.stdout) is not None:
			return True
		logging.error('error while creating buildconfig: ' + ret.stdout)
		return False

	def _recreate_is_tag(self, name, newTag, filename):
		ret = self.cmd.run(f'oc describe is {name}')
		if ret.returncode != 0:
			ret = self.cmd.run(f'oc create -f {filename}')
			if ret.returncode != 0:
				logging.error(f'error while creating imagestream: {ret.stdout}')
				return False
		else:
			logging.debug(f'-> imagestream {name} found')
		image = f'{name}:{newTag}'
		self.cmd.run(f'oc delete istag {image}', reportNonZero=False) # we don't care if this fails, e.g., if it is missing
		ret = self.cmd.run(f'oc create istag {image}')
		if ret.returncode == 0:
			return True
		logging.error(f'error while creating imagestreamtag: {ret.stdout}')
		return False

	def _start_build(self, name):
		# will return "immediately" but build runs in background
		# if multiple builds are started at the same time, this can take some time, however
		ret = self.cmd.run(f'oc start-build {name} --from-dir={self.eNBSourceCodePath} --exclude=""')
		regres = re.search(r'build.build.openshift.io/(?P<jobname>[a-zA-Z0-9\-]+) started', ret.stdout)
		if ret.returncode != 0 or ret.stdout.count('Uploading finished') != 1 or regres is None:
			logging.error(f"error during oc start-build: {ret.stdout}")
			return None
		return regres.group('jobname') + '-build'

	def _wait_build_end(self, jobs, timeout_sec, check_interval_sec = 5):
		logging.debug(f"waiting for jobs {jobs} to finish building")
		while timeout_sec > 0:
			# check status
			for j in jobs:
				ret = self.cmd.run(f'oc get pods | grep {j}', silent = True)
				if ret.stdout.count('Completed') > 0: jobs.remove(j)
				if ret.stdout.count('Error') > 0:
					logging.error(f'error for job {j}: {ret.stdout}')
					return False
			if jobs == []:
				logging.debug('all jobs completed')
				return True
			time.sleep(check_interval_sec)
			timeout_sec -= check_interval_sec
		logging.error(f"timeout while waiting for end of build of {jobs}")
		return False

	def _retag_image_statement(self, oldImage, newImage, newTag, filename):
		self.cmd.run(f'sed -i -e "s#{oldImage}:latest#{newImage}:{newTag}#" {filename}')

	def _get_image_size(self, image, tag):
		# get the SHA of the image we built using the image name and its tag
		ret = self.cmd.run(f'oc describe is {image} | grep -A4 {tag}')
		result = re.search(f'{IMAGE_REGISTRY_SERVICE_NAME}:5000/{NAMESPACE}/(?P<imageSha>{image}@sha256:[a-f0-9]+)', ret.stdout)
		if result is None:
			return -1
		imageSha = result.group("imageSha")

		# retrieve the size
		ret = self.cmd.run(f'oc get -o json isimage {imageSha} | jq -Mc "{{dockerImageSize: .image.dockerImageMetadata.Size}}"')
		result = re.search('{"dockerImageSize":(?P<size>[0-9]+)}', ret.stdout)
		if result is None:
			return -1
		return int(result.group("size"))

	def _deploy_pod(self, filename, timeout = 120):
		ret = self.cmd.run(f'oc create -f {filename}')
		result = re.search(r'pod/(?P<pod>[a-zA-Z0-9_\-]+) created', ret.stdout)
		if result is None:
			logging.error(f'could not deploy pod: {ret.stdout}')
			return None
		pod = result.group("pod")
		logging.debug(f'checking if pod {pod} is in Running state')
		ret = self.cmd.run(f'oc wait --for=condition=ready pod {pod} --timeout={timeout}s', silent=True)
		if ret.returncode == 0:
			return pod
		logging.error(f'pod {pod} did not reach Running state')
		self._undeploy_pod(filename)
		return None

	def _undeploy_pod(self, filename):
		self.cmd.run(f'oc delete -f {filename}')

	def PullClusterImage(self, HTML, node, images, tag_prefix):
		logging.debug(f'Pull OC image {images} to server {node}')
		self.testCase_id = HTML.testCase_id
		with cls_cmd.getConnection(node) as cmd:
			succeeded = OC_login(cmd, self.OCUserName, self.OCPassword, CI_OC_RAN_NAMESPACE)
			if not succeeded:
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			ret = cmd.run(f'oc whoami -t | docker login -u oaicicd --password-stdin {self.OCRegistry}')
			if ret.returncode != 0:
				logging.error(f'cannot authenticate at registry')
				OC_logout(cmd)
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
				return False
			tag = cls_containerize.CreateTag(self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			registry = f'{self.OCRegistry}/{CI_OC_RAN_NAMESPACE}'
			success, msg = cls_containerize.Containerize.Pull_Image(cmd, images, tag, tag_prefix, registry, None, None)
			OC_logout(cmd)
		param = f"on node {node}"
		if success:
			HTML.CreateHtmlTestRowQueue(param, 'OK', [msg])
		else:
			HTML.CreateHtmlTestRowQueue(param, 'KO', [msg])
		return success

	def BuildClusterImage(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			raise ValueError(f'Insufficient Parameter: ranRepository {self.ranRepository} ranBranch {ranBranch} ranCommitID {self.ranCommitID}')
		lIpAddr = self.eNBIPAddress
		lSourcePath = self.eNBSourceCodePath
		if lIpAddr == '' or lSourcePath == '':
			raise ValueError('Insufficient Parameter: eNBSourceCodePath missing')
		ocUserName = self.OCUserName
		ocPassword = self.OCPassword
		ocProjectName = self.OCProjectName
		if ocUserName == '' or ocPassword == '' or ocProjectName == '':
			HELP.GenericHelp(CONST.Version)
			raise ValueError('Insufficient Parameter: no OC Credentials')
		if self.OCRegistry.startswith("http") or self.OCRegistry.endswith("/"):
			raise ValueError(f'ocRegistry {self.OCRegistry} should not start with http:// or https:// and not end on a slash /')

		logging.debug(f'Building on cluster triggered from server: {lIpAddr}')
		self.cmd = cls_cmd.RemoteCmd(lIpAddr)

		self.testCase_id = HTML.testCase_id

		# Workaround for some servers, we need to erase completely the workspace
		self.cmd.cd(lSourcePath)
		# to reduce the amount of data send to OpenShift, we
		# manually delete all generated files in the workspace
		self.cmd.run(f'rm -rf {lSourcePath}/cmake_targets/ran_build');

		baseTag = 'develop'
		forceBaseImageBuild = False
		if self.ranAllowMerge: # merging MR branch into develop -> temporary image
			branchName = self.ranBranch.replace('/','-')
			imageTag = f'{branchName}-{self.ranCommitID[0:8]}'
			if self.ranTargetBranch == 'develop':
				ret = self.cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base.rhel9 | grep --colour=never -i INDEX')
				result = re.search('index', ret.stdout)
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
			# if the branch name contains integration_20xx_wyy, let rebuild ran-base
			result = re.search('integration_20([0-9]{2})_w([0-9]{2})', self.ranBranch)
			if not forceBaseImageBuild and result is not None:
				forceBaseImageBuild = True
				baseTag = 'ci-temp'
		else:
			imageTag = f'develop-{self.ranCommitID[0:8]}'
			forceBaseImageBuild = True

		# logging to OC Cluster and then switch to corresponding project
		ret = self.cmd.run(f'oc login -u {ocUserName} -p {ocPassword} --server {self.OCUrl}')
		if ret.returncode != 0:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			return False

		ret = self.cmd.run(f'oc project {ocProjectName}')
		if ret.returncode != 0:
			logging.error(f'\u001B[1mUnable to access OC project {ocProjectName}\u001B[0m')
			self.cmd.run('oc logout')
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_PROJECT_FAIL)
			return False

		# delete old images by Sagar Arora <sagar.arora@openairinterface.org>:
		# 1. retrieve all images and their timestamp
		# 2. awk retrieves those whose timestamp is older than 3 weeks
		# 3. issue delete command on corresponding istags (the images are dangling and will be cleaned by the registry)
		delete_cmd = "oc get istag -o go-template --template '{{range .items}}{{.metadata.name}} {{.metadata.creationTimestamp}}{{\"\\n\"}}{{end}}' | awk '$2 <= \"'$(date -d '-3weeks' -Ins --utc | sed 's/+0000/Z/')'\" { print $1 }' | xargs --no-run-if-empty oc delete istag"
		response = self.cmd.run(delete_cmd)
		logging.debug(f"deleted images:\n{response.stdout}")

		self._recreate_entitlements()

		status = True # flag to abandon compiling if any image fails
		attemptedImages = []
		if forceBaseImageBuild:
			self._recreate_is_tag('ran-base', baseTag, 'openshift/ran-base-is.yaml')
			self._recreate_bc('ran-base', baseTag, 'openshift/ran-base-bc.yaml')
			ranbase_job = self._start_build('ran-base')
			attemptedImages += ['ran-base']
			status = ranbase_job is not None and self._wait_build_end([ranbase_job], 1000)
			if not status: logging.error('failure during build of ran-base')
			self.cmd.run(f'oc logs {ranbase_job} &> cmake_targets/log/ran-base.log') # cannot use cmd.run because of redirect
			# recover logs by mounting image
			self._retag_image_statement('ran-base', 'ran-base', baseTag, 'openshift/ran-base-log-retrieval.yaml')
			pod = self._deploy_pod('openshift/ran-base-log-retrieval.yaml')
			if pod is not None:
				self.cmd.run(f'mkdir -p cmake_targets/log/ran-base')
				self.cmd.run(f'oc rsync {pod}:/oai-ran/cmake_targets/log/ cmake_targets/log/ran-base')
				self._undeploy_pod('openshift/ran-base-log-retrieval.yaml')
			else:
				status = False

		if status:
			self._recreate_is_tag('oai-physim', imageTag, 'openshift/oai-physim-is.yaml')
			self._recreate_bc('oai-physim', imageTag, 'openshift/oai-physim-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.phySim.rhel9')
			physim_job = self._start_build('oai-physim')
			attemptedImages += ['oai-physim']

			self._recreate_is_tag('ran-build', imageTag, 'openshift/ran-build-is.yaml')
			self._recreate_bc('ran-build', imageTag, 'openshift/ran-build-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.rhel9')
			ranbuild_job = self._start_build('ran-build')
			attemptedImages += ['ran-build']

			self._recreate_is_tag('oai-clang', imageTag, 'openshift/oai-clang-is.yaml')
			self._recreate_bc('oai-clang', imageTag, 'openshift/oai-clang-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.clang.rhel9')
			clang_job = self._start_build('oai-clang')
			attemptedImages += ['oai-clang']

			wait = ranbuild_job is not None and physim_job is not None and clang_job is not None and self._wait_build_end([ranbuild_job, physim_job, clang_job], 1200)
			if not wait: logging.error('error during build of ranbuild_job or physim_job or clang_job')
			status = status and wait
			self.cmd.run(f'oc logs {ranbuild_job} &> cmake_targets/log/ran-build.log')
			self.cmd.run(f'oc logs {physim_job} &> cmake_targets/log/oai-physim.log')
			self.cmd.run(f'oc logs {clang_job} &> cmake_targets/log/oai-clang.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log')

		if status:
			self._recreate_is_tag('oai-enb', imageTag, 'openshift/oai-enb-is.yaml')
			self._recreate_bc('oai-enb', imageTag, 'openshift/oai-enb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.eNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.eNB.rhel9')
			enb_job = self._start_build('oai-enb')
			attemptedImages += ['oai-enb']

			self._recreate_is_tag('oai-gnb', imageTag, 'openshift/oai-gnb-is.yaml')
			self._recreate_bc('oai-gnb', imageTag, 'openshift/oai-gnb-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.rhel9')
			gnb_job = self._start_build('oai-gnb')
			attemptedImages += ['oai-gnb']

			self._recreate_is_tag('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-is.yaml')
			self._recreate_bc('oai-gnb-aw2s', imageTag, 'openshift/oai-gnb-aw2s-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.gNB.aw2s.rhel9')
			gnb_aw2s_job = self._start_build('oai-gnb-aw2s')
			attemptedImages += ['oai-gnb-aw2s']

			wait = enb_job is not None and gnb_job is not None and gnb_aw2s_job is not None and self._wait_build_end([enb_job, gnb_job, gnb_aw2s_job], 800)
			if not wait: logging.error('error during build of eNB/gNB')
			status = status and wait
			# recover logs
			self.cmd.run(f'oc logs {enb_job} &> cmake_targets/log/oai-enb.log')
			self.cmd.run(f'oc logs {gnb_job} &> cmake_targets/log/oai-gnb.log')
			self.cmd.run(f'oc logs {gnb_aw2s_job} &> cmake_targets/log/oai-gnb-aw2s.log')

			self._recreate_is_tag('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-is.yaml')
			self._recreate_bc('oai-nr-cuup', imageTag, 'openshift/oai-nr-cuup-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nr-cuup.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nr-cuup.rhel9')
			nr_cuup_job = self._start_build('oai-nr-cuup')
			attemptedImages += ['oai-nr-cuup']

			self._recreate_is_tag('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-is.yaml')
			self._recreate_bc('oai-lte-ue', imageTag, 'openshift/oai-lte-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.lteUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.lteUE.rhel9')
			lteue_job = self._start_build('oai-lte-ue')
			attemptedImages += ['oai-lte-ue']

			self._recreate_is_tag('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-is.yaml')
			self._recreate_bc('oai-nr-ue', imageTag, 'openshift/oai-nr-ue-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.nrUE.rhel9')
			self._retag_image_statement('ran-build', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build', imageTag, 'docker/Dockerfile.nrUE.rhel9')
			nrue_job = self._start_build('oai-nr-ue')
			attemptedImages += ['oai-nr-ue']

			wait = nr_cuup_job is not None and lteue_job is not None and nrue_job is not None and self._wait_build_end([nr_cuup_job, lteue_job, nrue_job], 800)
			if not wait: logging.error('error during build of nr-cuup/lteUE/nrUE')
			status = status and wait
			# recover logs
			self.cmd.run(f'oc logs {nr_cuup_job} &> cmake_targets/log/oai-nr-cuup.log')
			self.cmd.run(f'oc logs {lteue_job} &> cmake_targets/log/oai-lte-ue.log')
			self.cmd.run(f'oc logs {nrue_job} &> cmake_targets/log/oai-nr-ue.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log')

		if status:
			self._recreate_is_tag('ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-is.yaml')
			self._recreate_bc('ran-build-fhi72', imageTag, 'openshift/ran-build-fhi72-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.build.fhi72.rhel9')
			ranbuildfhi72_job = self._start_build('ran-build-fhi72')
			attemptedImages += ['ran-build-fhi72']

			wait = ranbuildfhi72_job is not None and self._wait_build_end([ranbuildfhi72_job], 1200)
			if not wait: logging.error('error during build of ranbuildfhi72_job')
			status = status and wait
			self.cmd.run(f'oc logs {ranbuildfhi72_job} &> cmake_targets/log/ran-build-fhi72.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log')

		if status:
			self._recreate_is_tag('oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-is.yaml')
			self._recreate_bc('oai-gnb-fhi72', imageTag, 'openshift/oai-gnb-fhi72-bc.yaml')
			self._retag_image_statement('ran-base', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-base', baseTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			self._retag_image_statement('ran-build-fhi72', 'image-registry.openshift-image-registry.svc:5000/oaicicd-ran/ran-build-fhi72', imageTag, 'docker/Dockerfile.gNB.fhi72.rhel9')
			gnb_fhi72_job = self._start_build('oai-gnb-fhi72')
			attemptedImages += ['oai-gnb-fhi72']

			wait = gnb_fhi72_job is not None and self._wait_build_end([gnb_fhi72_job], 600)
			if not wait: logging.error('error during build of gNB-fhi72')
			status = status and wait
			# recover logs
			self.cmd.run(f'oc logs {gnb_fhi72_job} &> cmake_targets/log/oai-gnb-fhi72.log')
			self.cmd.run(f'oc get pods.metrics.k8s.io &>> cmake_targets/log/build-metrics.log')

		# split and analyze logs
		imageSize = {}
		for image in attemptedImages:
			self.cmd.run(f'mkdir -p cmake_targets/log/{image}')
			tag = imageTag if image != 'ran-base' else baseTag
			size = self._get_image_size(image, tag)
			if size <= 0:
				imageSize[image] = 'unknown -- BUILD FAILED'
				status = False
			else:
				sizeMb = float(size) / 1000000
				imageSize[image] = f'{sizeMb:.1f} Mbytes (uncompressed: ~{sizeMb*2.5:.1f} Mbytes)'
			logging.info(f'\u001B[1m{image} size is {imageSize[image]}\u001B[0m')

		grep_exp = r"\|".join(attemptedImages)
		self.cmd.run(f'oc get images | grep -e \'{grep_exp}\' &> cmake_targets/log/image_registry.log');
		self.cmd.run(f'for pod in $(oc get pods | tail -n +2 | awk \'{{print $1}}\'); do oc get pod $pod -o json &>> cmake_targets/log/build_pod_summary.log; done')

		build_log_name = f'build_log_{self.testCase_id}'
		cls_containerize.CopyLogsToExecutor(self.cmd, lSourcePath, build_log_name)

		self.cmd.run('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc delete pod ${pod}; done')

		# logout will return eventually, but we don't care when -> start in background
		self.cmd.run(f'oc logout')
		self.cmd.close()

		# Analyze the logs
		collectInfo = cls_containerize.AnalyzeBuildLogs(build_log_name, attemptedImages, status)
		for img in collectInfo:
			for f in collectInfo[img]:
				status = status and collectInfo[img][f]['status']
		if not status:
			logging.debug(collectInfo)

		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
			HTML.CreateHtmlTestRow('all', 'OK', CONST.ALL_PROCESSES_OK)
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow('all', 'KO', CONST.ALL_PROCESSES_OK)

		HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, imageSize)

		return status

	def deploy_oc_physim(self, HTML, oc_release, svr_id):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			raise ValueError(f'Insufficient Parameter: ranRepository {self.ranRepository} ranBranch {self.ranBranch} ranCommitID {self.ranCommitID}')
		image_tag = cls_containerize.CreateTag(self.ranCommitID, self.ranBranch, self.ranAllowMerge)
		logging.debug(f'Running physims from server: {svr_id}')
		script = "scripts/oc-deploy-physims.sh"
		options = f"oaicicd-core-for-ci-ran {oc_release} {image_tag} {self.eNBSourceCodePath}"
		ret = cls_cmd.runScript(svr_id, script, 600, options)
		logging.debug(f'"{script}" finished with code {ret.returncode}, output:\n{ret.stdout}')
		log_dir = f'{os.getcwd()}/../cmake_targets/log'
		os.makedirs(log_dir, exist_ok=True)
		result_junit = f'{oc_release}-run.xml'
		details_json = f'{oc_release}-tests.json'
		with cls_cmd.getConnection(svr_id) as ssh:
			ssh.copyin(src=f'{self.eNBSourceCodePath}/ci-scripts/{details_json}', tgt=f'{log_dir}/{details_json}')
			ssh.copyin(src=f'{self.eNBSourceCodePath}/ci-scripts/{result_junit}', tgt=f'{log_dir}/{result_junit}')
			ssh.copyin(src=f'{self.eNBSourceCodePath}/ci-scripts/physim_log.txt', tgt=f'{log_dir}/physim_log.txt')
			ssh.copyin(src=f'{self.eNBSourceCodePath}/ci-scripts/physim_pods_summary.txt', tgt=f'{log_dir}/physim_pods_summary.txt')
			ssh.copyin(src=f'{self.eNBSourceCodePath}/ci-scripts/LastTestsFailed.log', tgt=f'{log_dir}/LastTestsFailed.log')
		test_status, test_summary, test_result = cls_analysis.Analysis.analyze_oc_physim(f'{log_dir}/{result_junit}', f'{log_dir}/{details_json}')
		if test_summary:
			if test_status:
				HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
				HTML.CreateHtmlTestRowPhySimTestResult(test_summary, test_result)
				logging.info('\u001B[1m Physical Simulator Pass\u001B[0m')
			else:
				HTML.CreateHtmlTestRow('Some test(s) failed!', 'KO', CONST.OC_PHYSIM_DEPLOY_FAIL)
				HTML.CreateHtmlTestRowPhySimTestResult(test_summary, test_result)
				logging.error('\u001B[1m Physical Simulator Fail\u001B[0m')
		else:
			HTML.CreateHtmlTestRowQueue('Physical simulator failed', 'KO', [test_result])
			logging.error('\u001B[1m Physical Simulator Fail\u001B[0m')
		return test_status
