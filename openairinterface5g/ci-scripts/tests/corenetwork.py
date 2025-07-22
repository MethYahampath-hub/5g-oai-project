import sys
import logging
logging.basicConfig(
	level=logging.DEBUG,
	stream=sys.stdout,
	format="[%(asctime)s] %(levelname)8s: %(message)s"
)
import os

import unittest

sys.path.append('./') # to find OAI imports below
import cls_corenetwork
import cls_cmd

class TestCoreNetwork(unittest.TestCase):
    def test_simple_core(self):
        c = cls_corenetwork.CoreNetwork("test", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertTrue(success)
        self.assertEqual(output, "deploy")
        # should not have it set in config
        self.assertTrue(c.runIperf3Server())
        self.assertEqual(c.getIP(), "127.0.0.1")
        log_files, output = c.undeploy(log_dir="/tmp")
        self.assertEqual(output, "undeploy")
        with cls_cmd.LocalCmd() as cmd:
            # there must be one log file for this (test) core
            self.assertEqual(len(log_files), 1)
            # test core writes to %%log_dir%%/logs, which expands to /tmp/logs
            l = log_files[0]
            self.assertEqual(l, "/tmp/logs")
            ret = cmd.run(f"cat {l}")
            self.assertEqual(ret.returncode, 0) # command must succeed
            self.assertEqual(ret.stdout, "logs") # output should be "logs"

    def test_core_list(self):
        c = cls_corenetwork.CoreNetwork("test_list", filename="tests/config/test_core_infra.yaml")
        success, _ = c.deploy()
        self.assertTrue(success)
        self.assertFalse(c.runIperf3Server())
        c.undeploy(log_dir="/tmp")

    def test_core_fail(self):
        c = cls_corenetwork.CoreNetwork("test_fail", filename="tests/config/test_core_infra.yaml")
        success, _ = c.deploy()
        self.assertFalse(success)
        # undeployment should still work
        c.undeploy(log_dir="/tmp")

    def test_core_script(self):
        c = cls_corenetwork.CoreNetwork("test_script", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertTrue(success)
        self.assertEqual(output, "deployment from script")
        c.undeploy()

    def test_core_script_fail(self):
        c = cls_corenetwork.CoreNetwork("test_script_fail", filename="tests/config/test_core_infra.yaml")
        success, output = c.deploy()
        self.assertFalse(success)
        self.assertEqual(output, "deployment from script\nfailing")
        c.undeploy()

if __name__ == '__main__':
    unittest.main()
