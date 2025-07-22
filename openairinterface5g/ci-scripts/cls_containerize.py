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
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#
#   Required Python Package
#     pexpect
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import sys	      # arg
import re	       # reg
import logging
import os
import shutil
import time
from zipfile import ZipFile

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import cls_cmd
import helpreadme as HELP
import constants as CONST
import cls_oaicitest

#-----------------------------------------------------------
# Helper functions used here and in other classes
# (e.g., cls_cluster.py)
#-----------------------------------------------------------
IMAGES = ['oai-enb', 'oai-lte-ru', 'oai-lte-ue', 'oai-gnb', 'oai-nr-cuup', 'oai-gnb-aw2s', 'oai-nr-ue', 'oai-enb-asan', 'oai-gnb-asan', 'oai-lte-ue-asan', 'oai-nr-ue-asan', 'oai-nr-cuup-asan', 'oai-gnb-aerial', 'oai-gnb-fhi72']

def CreateWorkspace(host, sourcePath, ranRepository, ranCommitID, ranTargetBranch, ranAllowMerge):
	if ranCommitID == '':
		logging.error('need ranCommitID in CreateWorkspace()')
		raise ValueError('Insufficient Parameter in CreateWorkspace(): need ranCommitID')

	script = "scripts/create_workspace.sh"
	options = f"{sourcePath} {ranRepository} {ranCommitID}"
	if ranAllowMerge:
		if ranTargetBranch == '':
			ranTargetBranch = 'develop'
		options += f" {ranTargetBranch}"
	logging.info(f'execute "{script}" with options "{options}" on node {host}')
	ret = cls_cmd.runScript(host, script, 90, options)
	logging.debug(f'"{script}" finished with code {ret.returncode}, output:\n{ret.stdout}')
	return ret.returncode == 0

def CreateTag(ranCommitID, ranBranch, ranAllowMerge):
	if ranCommitID == 'develop':
		return 'develop'
	shortCommit = ranCommitID[0:8]
	if ranAllowMerge:
		# Allowing contributor to have a name/branchName format
		branchName = ranBranch.replace('/','-')
		tagToUse = f'{branchName}-{shortCommit}'
	else:
		tagToUse = f'develop-{shortCommit}'
	return tagToUse

def CopyLogsToExecutor(cmd, sourcePath, log_name):
	cmd.cd(f'{sourcePath}/cmake_targets')
	cmd.run(f'rm -f {log_name}.zip')
	cmd.run(f'mkdir -p {log_name}')
	cmd.run(f'mv log/* {log_name}')
	cmd.run(f'zip -r -qq {log_name}.zip {log_name}')

	# copy zip to executor for analysis
	if (os.path.isfile(f'./{log_name}.zip')):
		os.remove(f'./{log_name}.zip')
	if (os.path.isdir(f'./{log_name}')):
		shutil.rmtree(f'./{log_name}')
	cmd.copyin(src=f'{sourcePath}/cmake_targets/{log_name}.zip', tgt=f'{os.getcwd()}/{log_name}.zip')
	cmd.run(f'rm -f {log_name}.zip')
	ZipFile(f'{log_name}.zip').extractall('.')

def AnalyzeBuildLogs(buildRoot, images, globalStatus):
	collectInfo = {}
	for image in images:
		files = {}
		file_list = [f for f in os.listdir(f'{buildRoot}/{image}') if os.path.isfile(os.path.join(f'{buildRoot}/{image}', f)) and f.endswith('.txt')]
		# Analyze the "sub-logs" of every target image
		for fil in file_list:
			errorandwarnings = {}
			warningsNo = 0
			errorsNo = 0
			with open(f'{buildRoot}/{image}/{fil}', mode='r') as inputfile:
				for line in inputfile:
					result = re.search(' ERROR ', str(line))
					if result is not None:
						errorsNo += 1
					result = re.search(' error:', str(line))
					if result is not None:
						errorsNo += 1
					result = re.search(' WARNING ', str(line))
					if result is not None:
						warningsNo += 1
					result = re.search(' warning:', str(line))
					if result is not None:
						warningsNo += 1
				errorandwarnings['errors'] = errorsNo
				errorandwarnings['warnings'] = warningsNo
				errorandwarnings['status'] = globalStatus
			files[fil] = errorandwarnings
		# Analyze the target image
		if os.path.isfile(f'{buildRoot}/{image}.log'):
			errorandwarnings = {}
			committed = False
			tagged = False
			with open(f'{buildRoot}/{image}.log', mode='r') as inputfile:
				for line in inputfile:
					lineHasTag = re.search(f'Successfully tagged {image}:', str(line)) is not None
					lineHasTag2 = re.search(f'naming to docker.io/library/{image}:', str(line)) is not None
					tagged = tagged or lineHasTag or lineHasTag2
					# the OpenShift Cluster builder prepends image registry URL
					lineHasCommit = re.search(f'COMMIT [a-zA-Z0-9\.:/\-]*{image}', str(line)) is not None
					committed = committed or lineHasCommit
			errorandwarnings['errors'] = 0 if committed or tagged else 1
			errorandwarnings['warnings'] = 0
			errorandwarnings['status'] = committed or tagged
			files['Target Image Creation'] = errorandwarnings
		collectInfo[image] = files
	return collectInfo

def GetImageName(ssh, svcName, file):
	ret = ssh.run(f"docker compose -f {file} config --format json {svcName}  | jq -r '.services.\"{svcName}\".image'", silent=True)
	if ret.returncode != 0:
		return f"cannot retrieve image info for {containerName}: {ret.stdout}"
	else:
		return ret.stdout.strip()

def GetServiceHealth(ssh, svcName, file):
	if svcName is None:
		return False, f"Service {svcName} not found in {file}"
	image = GetImageName(ssh, svcName, file)
	if 'db_init' in svcName or 'db-init' in svcName: # exits with 0, there cannot be healthy
		return True, f"Service {svcName} healthy, image {image}"
	for _ in range(8):
		result = ssh.run(f"docker compose -f {file} ps --format json {svcName}  | jq -r 'if type==\"array\" then .[0].Health else .Health end'", silent=True)
		if result.stdout == 'healthy':
			return True, f"Service {svcName} healthy, image {image}"
		time.sleep(5)
	return False, f"Failed to deploy: service {svcName}"

def ExistEnvFilePrint(ssh, wd, prompt='env vars in existing'):
	ret = ssh.run(f'cat {wd}/.env', silent=True, reportNonZero=False)
	if ret.returncode != 0:
		return False
	env_vars = ret.stdout.strip().splitlines()
	logging.info(f'{prompt} {wd}/.env: {env_vars}')
	return True

def WriteEnvFile(ssh, services, wd, tag, flexric_tag):
	ret = ssh.run(f'cat {wd}/.env', silent=True, reportNonZero=False)
	registry = "oai-ci" # pull_images() gives us this registry path
	envs = {"REGISTRY":registry, "TAG": tag, "FLEXRIC_TAG": flexric_tag}
	if ret.returncode == 0: # it exists, we have to update
		# transforms env file to dictionary
		old_envs = {}
		for l in ret.stdout.strip().splitlines():
			var, val = l.split('=', 1)
			old_envs[var] = val.strip('"')
		# will retain the old environment variables
		envs = {**envs, **old_envs}
	for svc in services.split():
		# In some scenarios we have the choice of either pulling normal images
		# or -asan images. We need to detect which kind we did pull.
		fullImageName = GetImageName(ssh, svc, f"{wd}/docker-compose.y*ml")
		image = fullImageName.split("/")[-1].split(":")[0]
		checkimg = f"{registry}/{image}-asan:{tag}"
		ret = ssh.run(f'docker image inspect {checkimg}', reportNonZero=False)
		if ret.returncode == 0:
			logging.info(f"detected pulled image {checkimg}")
			if "oai-enb" in image: envs["ENB_IMG"] = "oai-enb-asan"
			elif "oai-gnb" in image: envs["GNB_IMG"] = "oai-gnb-asan"
			elif "oai-lte-ue" in image: envs["LTEUE_IMG"] = "oai-lte-ue-asan"
			elif "oai-nr-ue" in image: envs["NRUE_IMG"] = "oai-nr-ue-asan"
			elif "oai-nr-cuup" in image: envs["NRCUUP_IMG"] = "oai-nr-cuup-asan"
			else: logging.warning("undetected image format {image}, cannot use asan")
	env_string = "\n".join([f"{var}=\"{val}\"" for var,val in envs.items()])
	ssh.run(f'echo -e \'{env_string}\' > {wd}/.env', silent=True)
	ExistEnvFilePrint(ssh, wd, prompt='New env vars in file')

def GetServices(ssh, requested, file):
    if requested == [] or requested is None or requested == "":
        logging.warning('No service name given: starting all services in docker-compose.yml!')
        ret = ssh.run(f'docker compose -f {file} config --services')
        if ret.returncode != 0:
            return ""
        else:
            return ' '.join(ret.stdout.splitlines())
    else:
        return requested

def CopyinServiceLog(ssh, lSourcePath, yaml, svcName, wd_yaml, filename):
	remote_filename = f"{lSourcePath}/cmake_targets/log/{filename}"
	ssh.run(f'docker compose -f {wd_yaml} logs {svcName} --no-log-prefix &> {remote_filename}')
	local_dir = f"{os.getcwd()}/../cmake_targets/log/{yaml}"
	local_filename = f"{local_dir}/{filename}"
	return ssh.copyin(remote_filename, local_filename)

def GetRunningServices(ssh, file):
	ret = ssh.run(f'docker compose -f {file} config --services')
	if ret.returncode != 0:
		return None
	allServices = ret.stdout.splitlines()
	running_services = []
	for s in allServices:
		# outputs the hash if the container is running
		ret = ssh.run(f'docker compose -f {file} ps --all --quiet -- {s}')
		if ret.returncode != 0:
			logging.info(f"service {s}: {ret.stdout}")
		elif ret.stdout == "":
			logging.warning(f"could not retrieve information for service {s}")
		else:
			c = ret.stdout
			logging.debug(f'running service {s} with container id {c}')
			running_services.append((s, c))
	logging.info(f'stopping services: {running_services}')
	return running_services

def CheckLogs(self, yaml, service_name, HTML, RAN):
	logPath = f'{os.getcwd()}/../cmake_targets/log/{yaml}'
	filename = f'{logPath}/{service_name}-{HTML.testCase_id}.log'
	success = True
	if (any(sub in service_name for sub in ['oai_ue','oai-nr-ue','lte_ue'])):
		logging.debug(f'\u001B[1m Analyzing UE logfile {filename} \u001B[0m')
		logStatus = cls_oaicitest.OaiCiTest().AnalyzeLogFile_UE(filename, HTML, RAN)
		opt = f"UE log analysis for service {service_name}"
		# usage of htmlUEFailureMsg/htmleNBFailureMsg is because Analyze log files
		# abuse HTML to store their reports, and we here want to put custom options,
		# which is not possible with CreateHtmlTestRow
		# solution: use HTML templates, where we don't need different HTML write funcs
		if (logStatus < 0):
			HTML.CreateHtmlTestRowQueue(opt, 'KO', [HTML.htmlUEFailureMsg])
			success = False
		else:
			HTML.CreateHtmlTestRowQueue(opt, 'OK', [HTML.htmlUEFailureMsg])
		HTML.htmlUEFailureMsg = ""
	elif service_name == 'nv-cubb':
		msg = 'Undeploy PNF/Nvidia CUBB'
		HTML.CreateHtmlTestRow(msg, 'OK', CONST.ALL_PROCESSES_OK)
	elif (any(sub in service_name for sub in ['enb','rru','rcc','cu','du','gnb'])):
		logging.debug(f'\u001B[1m Analyzing XnB logfile {filename}\u001B[0m')
		logStatus = RAN.AnalyzeLogFile_eNB(filename, HTML, self.ran_checkers)
		opt = f"xNB log analysis for service {service_name}"
		if (logStatus < 0):
			HTML.CreateHtmlTestRowQueue(opt, 'KO', [HTML.htmleNBFailureMsg])
			success = False
		else:
			HTML.CreateHtmlTestRowQueue(opt, 'OK', [HTML.htmleNBFailureMsg])
		HTML.htmleNBFailureMsg = ""
	else:
		logging.info(f'Skipping to analyze log for service name {service_name}')
		HTML.CreateHtmlTestRowQueue(f"service {service_name}", 'OK', ["no analysis function"])
	logging.debug(f"log check: service {service_name} passed analysis {success}")
	return success

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class Containerize():

	def __init__(self):
		
		self.ranRepository = ''
		self.ranBranch = ''
		self.ranAllowMerge = False
		self.ranCommitID = ''
		self.ranTargetBranch = ''
		self.eNBIPAddress = ''
		self.eNBUserName = ''
		self.eNBPassword = ''
		self.eNBSourceCodePath = ''
		self.eNB1IPAddress = ''
		self.eNB1UserName = ''
		self.eNB1Password = ''
		self.eNB1SourceCodePath = ''
		self.eNB2IPAddress = ''
		self.eNB2UserName = ''
		self.eNB2Password = ''
		self.eNB2SourceCodePath = ''
		self.forcedWorkspaceCleanup = False
		self.imageKind = ''
		self.proxyCommit = None
		self.eNB_instance = 0
		self.eNB_serverId = ['', '', '']
		self.yamlPath = ['', '', '']
		self.services = ['', '', '']
		self.deploymentTag = ''
		self.eNB_logFile = ['', '', '']

		self.testCase_id = ''

		self.cli = ''
		self.cliBuildOptions = ''
		self.dockerfileprefix = ''
		self.host = ''

		self.deployedContainers = []
		self.tsharkStarted = False
		self.pingContName = ''
		self.pingOptions = ''
		self.pingLossThreshold = ''
		self.svrContName = ''
		self.svrOptions = ''
		self.cliContName = ''
		self.cliOptions = ''

		self.imageToCopy = ''
		#checkers from xml
		self.ran_checkers={}
		self.num_attempts = 1

		self.flexricTag = ''

#-----------------------------------------------------------
# Container management functions
#-----------------------------------------------------------

	def GetCredentials(self, server_id):
		if server_id == '0':
			ip, path = self.eNBIPAddress, self.eNBSourceCodePath
		elif server_id == '1':
			ip, path = self.eNB1IPAddress, self.eNB1SourceCodePath
		elif server_id == '2':
			ip, path = self.eNB2IPAddress, self.eNB2SourceCodePath
		else:
			raise ValueError(f"unknown server ID '{server_id}'")
		if ip == '' or path == '':
			HELP.GenericHelp(CONST.Version)
			raise ValueError(f'Insufficient Parameter: IP/node {ip}, path {path}')
		return (ip, path)

	def BuildImage(self, HTML):
		svr = self.eNB_serverId[self.eNB_instance]
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		logging.debug('Building on server: ' + lIpAddr)
		cmd = cls_cmd.getConnection(lIpAddr)
	
		# Checking the hostname to get adapted on cli and dockerfileprefixes
		cmd.run('hostnamectl')
		result = re.search('Ubuntu|Red Hat', cmd.getBefore())
		self.host = result.group(0)
		if self.host == 'Ubuntu':
			self.cli = 'docker'
			self.dockerfileprefix = '.ubuntu22'
			self.cliBuildOptions = ''
		elif self.host == 'Red Hat':
			self.cli = 'sudo podman'
			self.dockerfileprefix = '.rhel9'
			self.cliBuildOptions = '--disable-compression'

		# we always build the ran-build image with all targets
		# Creating a tupple with the imageName, the DockerFile prefix pattern, targetName and sanitized option
		imageNames = [('ran-build', 'build', 'ran-build', '')]
		result = re.search('eNB', self.imageKind)
		if result is not None:
			imageNames.append(('oai-enb', 'eNB', 'oai-enb', ''))
		result = re.search('gNB', self.imageKind)
		if result is not None:
			imageNames.append(('oai-gnb', 'gNB', 'oai-gnb', ''))
		result = re.search('all', self.imageKind)
		if result is not None:
			imageNames.append(('oai-enb', 'eNB', 'oai-enb', ''))
			imageNames.append(('oai-gnb', 'gNB', 'oai-gnb', ''))
			imageNames.append(('oai-nr-cuup', 'nr-cuup', 'oai-nr-cuup', ''))
			imageNames.append(('oai-lte-ue', 'lteUE', 'oai-lte-ue', ''))
			imageNames.append(('oai-nr-ue', 'nrUE', 'oai-nr-ue', ''))
			if self.host == 'Red Hat':
				imageNames.append(('oai-physim', 'phySim', 'oai-physim', ''))
			if self.host == 'Ubuntu':
				imageNames.append(('oai-lte-ru', 'lteRU', 'oai-lte-ru', ''))
				imageNames.append(('oai-gnb-aerial', 'gNB.aerial', 'oai-gnb-aerial', ''))
				# Building again the 5G images with Address Sanitizer
				imageNames.append(('ran-build', 'build', 'ran-build-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-enb', 'eNB', 'oai-enb-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-gnb', 'gNB', 'oai-gnb-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-lte-ue', 'lteUE', 'oai-lte-ue-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-nr-ue', 'nrUE', 'oai-nr-ue-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-nr-cuup', 'nr-cuup', 'oai-nr-cuup-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('ran-build-fhi72', 'build.fhi72', 'ran-build-fhi72', ''))
				imageNames.append(('oai-gnb', 'gNB.fhi72', 'oai-gnb-fhi72', ''))
		result = re.search('build_cross_arm64', self.imageKind)
		if result is not None:
			self.dockerfileprefix = '.ubuntu22.cross-arm64'
		result = re.search('native_arm', self.imageKind)
		if result is not None:
			imageNames.append(('oai-gnb', 'gNB', 'oai-gnb', ''))
			imageNames.append(('ran-build-fhi72', 'build.fhi72.native_arm', 'ran-build-fhi72', ''))
			imageNames.append(('oai-nr-cuup', 'nr-cuup', 'oai-nr-cuup', ''))
			imageNames.append(('oai-nr-ue', 'nrUE', 'oai-nr-ue', ''))
			imageNames.append(('oai-gnb-aerial', 'gNB.aerial', 'oai-gnb-aerial', ''))
		
		self.testCase_id = HTML.testCase_id
		cmd.cd(lSourcePath)
		# if asterix, copy the entitlement and subscription manager configurations
		if self.host == 'Red Hat':
			cmd.run('mkdir -p ./etc-pki-entitlement')
			cmd.run('cp /etc/pki/entitlement/*.pem ./etc-pki-entitlement/')

		baseImage = 'ran-base'
		baseTag = 'develop'
		forceBaseImageBuild = False
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
			if self.ranTargetBranch == 'develop':
				cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base{self.dockerfileprefix} | grep --colour=never -i INDEX')
				result = re.search('index', cmd.getBefore())
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
			# if the branch name contains integration_20xx_wyy, let rebuild ran-base
			result = re.search('integration_20([0-9]{2})_w([0-9]{2})', self.ranBranch)
			if not forceBaseImageBuild and result is not None:
				forceBaseImageBuild = True
				baseTag = 'ci-temp'
		else:
			forceBaseImageBuild = True

		# Let's remove any previous run artifacts if still there
		cmd.run(f"{self.cli} image prune --force")
		for image,pattern,name,option in imageNames:
			cmd.run(f"{self.cli} image rm {name}:{imageTag}")

		# Build the base image only on Push Events (not on Merge Requests)
		# On when the base image docker file is being modified.
		if forceBaseImageBuild:
			cmd.run(f"{self.cli} image rm {baseImage}:{baseTag}")
			cmd.run(f"{self.cli} build {self.cliBuildOptions} --target {baseImage} --tag {baseImage}:{baseTag} --file docker/Dockerfile.base{self.dockerfileprefix} . &> cmake_targets/log/ran-base.log", timeout=1600)
		# First verify if the base image was properly created.
		ret = cmd.run(f"{self.cli} image inspect --format=\'Size = {{{{.Size}}}} bytes\' {baseImage}:{baseTag}")
		allImagesSize = {}
		if ret.returncode != 0:
			logging.error('\u001B[1m Could not build properly ran-base\u001B[0m')
			# Recover the name of the failed container?
			cmd.run(f"{self.cli} ps --quiet --filter \"status=exited\" -n1 | xargs --no-run-if-empty {self.cli} rm -f")
			cmd.run(f"{self.cli} image prune --force")
			cmd.close()
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			return False
		else:
			result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', cmd.getBefore())
			if result is not None:
				size = float(result.group("size")) / 1000000
				imageSizeStr = f'{size:.1f}'
				logging.debug(f'\u001B[1m   ran-base size is {imageSizeStr} Mbytes\u001B[0m')
				allImagesSize['ran-base'] = f'{imageSizeStr} Mbytes'
			else:
				logging.debug('ran-base size is unknown')

		# Recover build logs, for the moment only possible when build is successful
		cmd.run(f"{self.cli} create --name test {baseImage}:{baseTag}")
		cmd.run("mkdir -p cmake_targets/log/ran-base")
		cmd.run(f"{self.cli} cp test:/oai-ran/cmake_targets/log/. cmake_targets/log/ran-base")
		cmd.run(f"{self.cli} rm -f test")

		# Build the target image(s)
		status = True
		attemptedImages = ['ran-base']
		for image,pattern,name,option in imageNames:
			attemptedImages += [name]
			# the archived Dockerfiles have "ran-base:latest" as base image
			# we need to update them with proper tag
			cmd.run(f'git checkout -- docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			cmd.run(f'sed -i -e "s#{baseImage}:latest#{baseImage}:{baseTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			# target images should use the proper ran-build image
			if image != 'ran-build' and "-asan" in name:
				cmd.run(f'sed -i -e "s#ran-build:latest#ran-build-asan:{imageTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			elif "fhi72" in name:
				cmd.run(f'sed -i -e "s#ran-build-fhi72:latest#ran-build-fhi72:{imageTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			elif image != 'ran-build':
				cmd.run(f'sed -i -e "s#ran-build:latest#ran-build:{imageTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			if image == 'oai-gnb-aerial':
				cmd.run('cp -f /opt/nvidia-ipc/nvipc_src.*.tar.gz .')
			ret = cmd.run(f'{self.cli} build {self.cliBuildOptions} --target {image} --tag {name}:{imageTag} --file docker/Dockerfile.{pattern}{self.dockerfileprefix} {option} . > cmake_targets/log/{name}.log 2>&1', timeout=1200)
			if image == 'oai-gnb-aerial':
				cmd.run('rm -f nvipc_src.*.tar.gz')
			if image == 'ran-build' and ret.returncode == 0:
				cmd.run(f"docker run --name test-log -d {name}:{imageTag} /bin/true")
				cmd.run(f"docker cp test-log:/oai-ran/cmake_targets/log/ cmake_targets/log/{name}/")
				cmd.run(f"docker rm -f test-log")
			else:
				cmd.run(f"mkdir -p cmake_targets/log/{name}")
			# check the status of the build
			ret = cmd.run(f"{self.cli} image inspect --format=\'Size = {{{{.Size}}}} bytes\' {name}:{imageTag}")
			if ret.returncode != 0:
				logging.error('\u001B[1m Could not build properly ' + name + '\u001B[0m')
				status = False
				# Here we should check if the last container corresponds to a failed command and destroy it
				cmd.run(f"{self.cli} ps --quiet --filter \"status=exited\" -n1 | xargs --no-run-if-empty {self.cli} rm -f")
				allImagesSize[name] = 'N/A -- Build Failed'
				break
			else:
				result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', cmd.getBefore())
				if result is not None:
					size = float(result.group("size")) / 1000000 # convert to MB
					imageSizeStr = f'{size:.1f}'
					logging.debug(f'\u001B[1m   {name} size is {imageSizeStr} Mbytes\u001B[0m')
					allImagesSize[name] = f'{imageSizeStr} Mbytes'
				else:
					logging.debug(f'{name} size is unknown')
					allImagesSize[name] = 'unknown'
			# Now pruning dangling images in between target builds
			cmd.run(f"{self.cli} image prune --force")

		# Remove all intermediate build images and clean up
		cmd.run(f"{self.cli} image rm ran-build:{imageTag} ran-build-asan:{imageTag} ran-build-fhi72:{imageTag} || true")
		cmd.run(f"{self.cli} volume prune --force")

		# Remove some cached artifacts to prevent out of diskspace problem
		logging.debug(cmd.run("df -h").stdout)
		logging.debug(cmd.run("docker system df").stdout)
		cmd.run(f"{self.cli} buildx prune --filter until=1h --force")
		logging.debug(cmd.run("df -h").stdout)
		logging.debug(cmd.run("docker system df").stdout)

		# create a zip with all logs
		build_log_name = f'build_log_{self.testCase_id}'
		CopyLogsToExecutor(cmd, lSourcePath, build_log_name)
		cmd.close()

		# Analyze the logs
		collectInfo = AnalyzeBuildLogs(build_log_name, attemptedImages, status)
		
		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)
			return True
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)
			HTML.CreateHtmlTabFooter(False)
			return False

	def BuildProxy(self, HTML):
		svr = self.eNB_serverId[self.eNB_instance]
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		logging.debug('Building on server: ' + lIpAddr)
		ssh = cls_cmd.getConnection(lIpAddr)

		self.testCase_id = HTML.testCase_id
		oldRanCommidID = self.ranCommitID
		oldRanRepository = self.ranRepository
		oldRanAllowMerge = self.ranAllowMerge
		oldRanTargetBranch = self.ranTargetBranch
		self.ranCommitID = self.proxyCommit
		self.ranRepository = 'https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy.git'
		self.ranAllowMerge = False
		self.ranTargetBranch = 'master'

		# Let's remove any previous run artifacts if still there
		ssh.run('docker image prune --force')
		# Remove any previous proxy image
		ssh.run('docker image rm oai-lte-multi-ue-proxy:latest')

		tag = self.proxyCommit
		logging.debug('building L2sim proxy image for tag ' + tag)
		# check if the corresponding proxy image with tag exists. If not, build it
		ret = ssh.run(f'docker image inspect --format=\'Size = {{{{.Size}}}} bytes\' proxy:{tag}')
		buildProxy = ret.returncode != 0 # if no image, build new proxy
		if buildProxy:
			ssh.run(f'rm -Rf {lSourcePath}')
			success = CreateWorkspace(lIpAddr, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)
			if not success:
				raise Exception("could not clone proxy repository")

			filename = f'build_log_{self.testCase_id}'
			fullpath = f'{lSourcePath}/{filename}'

			ssh.run(f'docker build --target oai-lte-multi-ue-proxy --tag proxy:{tag} --file {lSourcePath}/docker/Dockerfile.ubuntu18.04 {lSourcePath} > {fullpath} 2>&1')
			ssh.run(f'zip -r -qq {fullpath}.zip {fullpath}')
			local_file = f"{os.getcwd()}/../cmake_targets/log/{filename}.zip"
			ssh.copyin(f'{fullpath}.zip', local_file)
			# don't delete such that we might recover the zips
			#ssh.run(f'rm -f {fullpath}.zip')

			ssh.run('docker image prune --force')
			ret = ssh.run(f'docker image inspect --format=\'Size = {{{{.Size}}}} bytes\' proxy:{tag}')
			if ret.returncode != 0:
				logging.error('\u001B[1m Build of L2sim proxy failed\u001B[0m')
				ssh.close()
				HTML.CreateHtmlTestRow('commit ' + tag, 'KO', CONST.ALL_PROCESSES_OK)
				HTML.CreateHtmlTabFooter(False)
				return False
		else:
			logging.debug('L2sim proxy image for tag ' + tag + ' already exists, skipping build')

		# retag the build images to that we pick it up later
		ssh.run(f'docker image tag proxy:{tag} oai-lte-multi-ue-proxy:latest')

		# we assume that the host on which this is built will also run the proxy. The proxy
		# currently requires the following command, and the docker-compose up mechanism of
		# the CI does not allow to run arbitrary commands. Note that the following actually
		# belongs to the deployment, not the build of the proxy...
		logging.warning('the following command belongs to deployment, but no mechanism exists to exec it there!')
		ssh.run('sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up')

		# to prevent accidentally overwriting data that might be used later
		self.ranCommitID = oldRanCommidID
		self.ranRepository = oldRanRepository
		self.ranAllowMerge = oldRanAllowMerge
		self.ranTargetBranch = oldRanTargetBranch

		# we do not analyze the logs (we assume the proxy builds fine at this stage),
		# but need to have the following information to correctly display the HTML
		files = {}
		errorandwarnings = {}
		errorandwarnings['errors'] = 0
		errorandwarnings['warnings'] = 0
		errorandwarnings['status'] = True
		files['Target Image Creation'] = errorandwarnings
		collectInfo = {}
		collectInfo['proxy'] = files
		ret = ssh.run(f'docker image inspect --format=\'Size = {{{{.Size}}}} bytes\' proxy:{tag}')
		result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', ret.stdout)
		# Cleaning any created tmp volume
		ssh.run('docker volume prune --force')
		ssh.close()

		allImagesSize = {}
		if result is not None:
			imageSize = float(result.group('size')) / 1000000
			logging.debug('\u001B[1m   proxy size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
			allImagesSize['proxy'] = str(round(imageSize,1)) + ' Mbytes'
			logging.info('\u001B[1m Building L2sim Proxy Image Pass\u001B[0m')
			HTML.CreateHtmlTestRow('commit ' + tag, 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)
			return True
		else:
			logging.error('proxy size is unknown')
			allImagesSize['proxy'] = 'unknown'
			logging.error('\u001B[1m Build of L2sim proxy failed\u001B[0m')
			HTML.CreateHtmlTestRow('commit ' + tag, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			return False

	def BuildRunTests(self, HTML):
		svr = self.eNB_serverId[self.eNB_instance]
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		logging.debug('Building on server: ' + lIpAddr)
		cmd = cls_cmd.RemoteCmd(lIpAddr)
		cmd.cd(lSourcePath)

		ret = cmd.run('hostnamectl')
		result = re.search('Ubuntu', ret.stdout)
		host = result.group(0)
		if host != 'Ubuntu':
			cmd.close()
			raise Exception("Can build unit tests only on Ubuntu server")
		logging.debug('running on Ubuntu as expected')

		if self.forcedWorkspaceCleanup:
			cmd.run(f'sudo -S rm -Rf {lSourcePath}')
		self.testCase_id = HTML.testCase_id
	
		# check that ran-base image exists as we expect it
		baseImage = 'ran-base'
		baseTag = 'develop'
		if self.ranAllowMerge:
			if self.ranTargetBranch == 'develop':
				cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base{self.dockerfileprefix} | grep --colour=never -i INDEX')
				result = re.search('index', cmd.getBefore())
				if result is not None:
					baseTag = 'ci-temp'
		ret = cmd.run(f"docker image inspect --format=\'Size = {{{{.Size}}}} bytes\' {baseImage}:{baseTag}")
		if ret.returncode != 0:
			logging.error(f'No {baseImage} image present, cannot build tests')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			return False

		# build ran-unittests image
		dockerfile = "ci-scripts/docker/Dockerfile.unittest.ubuntu22"
		ret = cmd.run(f'docker build --progress=plain --tag ran-unittests:{baseTag} --file {dockerfile} . &> {lSourcePath}/cmake_targets/log/unittest-build.log')
		if ret.returncode != 0:
			build_log_name = f'build_log_{self.testCase_id}'
			CopyLogsToExecutor(cmd, lSourcePath, build_log_name)
			logging.error(f'Cannot build unit tests')
			HTML.CreateHtmlTestRow("Unit test build failed", 'KO', [dockerfile])
			HTML.CreateHtmlTabFooter(False)
			return False

		HTML.CreateHtmlTestRowQueue("Build unit tests", 'OK', [dockerfile])

		# it worked, build and execute tests, and close connection
		ret = cmd.run(f'docker run -a STDOUT --workdir /oai-ran/build/ --env LD_LIBRARY_PATH=/oai-ran/build/ --rm ran-unittests:{baseTag} ctest --output-on-failure --no-label-summary -j$(nproc)')
		cmd.run(f'docker rmi ran-unittests:{baseTag}')
		build_log_name = f'build_log_{self.testCase_id}'
		CopyLogsToExecutor(cmd, lSourcePath, build_log_name)
		cmd.close()

		if ret.returncode == 0:
			HTML.CreateHtmlTestRowQueue('Unit tests succeeded', 'OK', [ret.stdout])
			HTML.CreateHtmlTabFooter(True)
			return True
		else:
			HTML.CreateHtmlTestRowQueue('Unit tests failed (see also doc/UnitTests.md)', 'KO', [ret.stdout])
			HTML.CreateHtmlTabFooter(False)
			return False

	def Push_Image_to_Local_Registry(self, HTML, svr_id, tag_prefix=""):
		lIpAddr, lSourcePath = self.GetCredentials(svr_id)
		logging.debug('Pushing images to server: ' + lIpAddr)
		ssh = cls_cmd.getConnection(lIpAddr)
		imagePrefix = 'porcepix.sboai.cs.eurecom.fr'
		ret = ssh.run(f'docker login -u oaicicd -p oaicicd {imagePrefix}')
		if ret.returncode != 0:
			msg = 'Could not log into local registry'
			logging.error(msg)
			ssh.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False

		orgTag = 'develop'
		if self.ranAllowMerge:
			orgTag = 'ci-temp'
		for image in IMAGES:
			tagToUse = tag_prefix + CreateTag(self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			imageTag = f"{image}:{tagToUse}"
			ret = ssh.run(f'docker image tag {image}:{orgTag} {imagePrefix}/{imageTag}')
			if ret.returncode != 0:
				continue
			ret = ssh.run(f'docker push {imagePrefix}/{imageTag}')
			if ret.returncode != 0:
				msg = f'Could not push {image} to local registry : {imageTag}'
				logging.error(msg)
				ssh.close()
				HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
				return False
			# Creating a develop tag on the local private registry
			if not self.ranAllowMerge:
				devTag = f"{tag_prefix}develop"
				ssh.run(f'docker image tag {image}:{orgTag} {imagePrefix}/{image}:{devTag}')
				ssh.run(f'docker push {imagePrefix}/{image}:{devTag}')
				ssh.run(f'docker rmi {imagePrefix}/{image}:{devTag}')
			ssh.run(f'docker rmi {imagePrefix}/{imageTag} {image}:{orgTag}')

		ret = ssh.run(f'docker logout {imagePrefix}')
		if ret.returncode != 0:
			msg = 'Could not log off from local registry'
			logging.error(msg)
			ssh.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False

		ssh.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		return True

	def Pull_Image(cmd, images, tag, tag_prefix, registry, username, password):
		if username is not None and password is not None:
			logging.info(f"logging into registry {username}@{registry}")
			response = cmd.run(f'docker login -u {username} -p {password} {registry}', silent=True, reportNonZero=False)
			if response.returncode != 0:
				msg = f'Could not log into registry {username}@{registry}'
				logging.error(msg)
				return False, msg
		pulled_images = []
		for image in images:
			imagePrefTag = f"{image}:{tag_prefix}{tag}"
			imageTag = f"{image}:{tag}"
			response = cmd.run(f'docker pull {registry}/{imagePrefTag}')
			if response.returncode != 0:
				msg = f'Could not pull {image} from local registry: {imagePrefTag}'
				logging.error(msg)
				return False, msg
			cmd.run(f'docker tag {registry}/{imagePrefTag} oai-ci/{imageTag}')
			cmd.run(f'docker rmi {registry}/{imagePrefTag}')
			pulled_images += [f"oai-ci/{imageTag}"]
		if username is not None and password is not None:
			response = cmd.run(f'docker logout {registry}')
			# we have the images, if logout fails it's no problem
		msg = "Pulled Images:\n" + '\n'.join(pulled_images)
		return True, msg

	def Pull_Image_from_Registry(self, HTML, svr_id, images, tag=None, tag_prefix="", registry="porcepix.sboai.cs.eurecom.fr", username="oaicicd", password="oaicicd"):
		lIpAddr, lSourcePath = self.GetCredentials(svr_id)
		logging.debug('\u001B[1m Pulling image(s) on server: ' + lIpAddr + '\u001B[0m')
		if not tag:
			tag = CreateTag(self.ranCommitID, self.ranBranch, self.ranAllowMerge)
		with cls_cmd.getConnection(lIpAddr) as cmd:
			success, msg = Containerize.Pull_Image(cmd, images, tag, tag_prefix, registry, username, password)
		param = f"on node {lIpAddr}"
		if success:
			HTML.CreateHtmlTestRowQueue(param, 'OK', [msg])
		else:
			HTML.CreateHtmlTestRowQueue(param, 'KO', [msg])
		return success

	def Clean_Test_Server_Images(self, HTML, svr_id, images, tag=None):
		lIpAddr, lSourcePath = self.GetCredentials(svr_id)
		logging.debug(f'\u001B[1m Cleaning image(s) from server: {lIpAddr}\u001B[0m')
		if not tag:
			tag = CreateTag(self.ranCommitID, self.ranBranch, self.ranAllowMerge)

		status = True
		with cls_cmd.getConnection(lIpAddr) as myCmd:
			removed_images = []
			for image in images:
				fullImage = f"oai-ci/{image}:{tag}"
				cmd = f'docker rmi {fullImage}'
				if myCmd.run(cmd).returncode != 0:
					status = False
				removed_images += [fullImage]

		msg = "Removed Images:\n" + '\n'.join(removed_images)
		s = 'OK' if status else 'KO'
		param = f"on node {lIpAddr}"
		HTML.CreateHtmlTestRowQueue(param, s, [msg])
		return status

	def Create_Workspace(self,HTML):
		svr = self.eNB_serverId[self.eNB_instance]
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		success = CreateWorkspace(lIpAddr, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)
		if success:
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', [f"created workspace {lSourcePath}"])
		else:
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["cannot create workspace"])
		return success

	def DeployObject(self, HTML):
		svr = self.eNB_serverId[self.eNB_instance]
		num_attempts = self.num_attempts
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		logging.debug(f'Deploying OAI Object on server: {lIpAddr}')
		yaml = self.yamlPath[self.eNB_instance].strip('/')
		# creating the log folder by default
		local_dir = f"{os.getcwd()}/../cmake_targets/log/{yaml.split('/')[-1]}"
		os.system(f'mkdir -p {local_dir}')
		wd = f'{lSourcePath}/{yaml}'
		wd_yaml = f'{wd}/docker-compose.y*ml'
		yaml_dir = yaml.split('/')[-1]
		with cls_cmd.getConnection(lIpAddr) as ssh:
			services = GetServices(ssh, self.services[self.eNB_instance], wd_yaml)
			if services == [] or services == ' ' or services == None:
				msg = 'Cannot determine services to start'
				logging.error(msg)
				HTML.CreateHtmlTestRowQueue('N/A', 'KO', [msg])
				return False
			ExistEnvFilePrint(ssh, wd)
			WriteEnvFile(ssh, services, wd, self.deploymentTag, self.flexricTag)
			if num_attempts <= 0:
				raise ValueError(f'Invalid value for num_attempts: {num_attempts}, must be greater than 0')
			for attempt in range(num_attempts):
				imagesInfo = []
				healthInfo = []
				logging.info(f'will start services {services}')
				status = ssh.run(f'docker compose -f {wd_yaml} up -d -- {services}')
				if status.returncode != 0:
					msg = f'cannot deploy services {services}: {status.stdout}'
					logging.error(msg)
					HTML.CreateHtmlTestRowQueue('N/A', 'NOK', [msg])
					return False
				for svc in services.split():
					health, msg = GetServiceHealth(ssh, svc, f'{wd_yaml}')
					logging.info(msg)
					imagesInfo.append(msg)
					healthInfo.append(health)
				deployed = all(healthInfo)
				if deployed:
					break
				elif (attempt < num_attempts - 1):
					warning_msg = f'Restart services {services}'
					logging.warning(warning_msg)
					imagesInfo.append(warning_msg)
					HTML.CreateHtmlTestRowQueue('N/A', 'NOK', ['\n'.join(imagesInfo)])
					for svc in services.split():
						CopyinServiceLog(ssh, lSourcePath, yaml_dir, svc, wd_yaml, f'{svc}-{HTML.testCase_id}-attempt{attempt}.log')
					ssh.run(f'docker compose -f {wd_yaml} down -- {services}')
		if deployed:
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', ['\n'.join(imagesInfo)])
		else:
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ['\n'.join(imagesInfo)])
		return deployed

	def UndeployObject(self, HTML, RAN):
		svr = self.eNB_serverId[self.eNB_instance]
		lIpAddr, lSourcePath = self.GetCredentials(svr)
		logging.debug(f'\u001B[1m Undeploying OAI Object from server: {lIpAddr}\u001B[0m')
		yaml = self.yamlPath[self.eNB_instance].strip('/')
		wd = f'{lSourcePath}/{yaml}'
		yaml_dir = yaml.split('/')[-1]
		with cls_cmd.getConnection(lIpAddr) as ssh:
			ExistEnvFilePrint(ssh, wd)
			services = GetRunningServices(ssh, f"{wd}/docker-compose.y*ml")
			copyin_res = None
			if services is not None:
				all_serv = " ".join([s for s, _ in services])
				ssh.run(f'docker compose -f {wd}/docker-compose.y*ml stop -- {all_serv}')
				copyin_res = all(CopyinServiceLog(ssh, lSourcePath, yaml_dir, s, f"{wd}/docker-compose.y*ml", f'{s}-{HTML.testCase_id}.log') for s, c in services)
			else:
				logging.warning('could not identify services to stop => no log file')
			ssh.run(f'docker compose -f {wd}/docker-compose.y*ml down -v')
			ssh.run(f'rm {wd}/.env')
		if not copyin_res:
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ['Could not copy logfile(s)'])
			return False
		else:
			log_results = [CheckLogs(self, yaml_dir, s, HTML, RAN) for s, _ in services]
			success = all(log_results)
		if success:
			logging.info('\u001B[1m Undeploying OAI Object Pass\u001B[0m')
		else:
			logging.error('\u001B[1m Undeploying OAI Object Failed\u001B[0m')
		return success
