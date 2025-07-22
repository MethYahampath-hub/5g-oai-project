import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import unittest
import tempfile

sys.path.append('./') # to find OAI imports below
import cls_oai_html
import cls_containerize
import cls_cmd

class TestBuild(unittest.TestCase):
	def setUp(self):
		self.html = cls_oai_html.HTMLManagement()
		self.html.testCase_id = "000000"
		self.cont = cls_containerize.Containerize()
		self.cont.eNB_serverId[0] = '0'
		self.cont.eNBIPAddress = 'localhost'
		self.cont.eNBUserName = None
		self.cont.eNBPassword = None
		self._d = tempfile.mkdtemp()
		logging.warning(f"temporary directory: {self._d}")
		self.cont.eNBSourceCodePath = self._d

	def tearDown(self):
		logging.warning(f"removing directory contents")
		with cls_cmd.getConnection(None) as cmd:
			cmd.run(f"rm -rf {self._d}")

	def test_build_proxy(self):
		self.cont.proxyCommit = "b64d9bce986b38ca59e8582864ade3fcdd05c0dc"
		success = self.cont.BuildProxy(self.html)
		self.assertTrue(success)

if __name__ == '__main__':
	unittest.main()
