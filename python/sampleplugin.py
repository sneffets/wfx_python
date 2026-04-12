from PluginAPI import *

class SamplePlugin(WfxPlugin):

    def __init__(self):
        tcbridge.debug_msg('python __init__ called')
        # self.a = []

    def set_ini_path(self, ini_path: str):
        tcbridge.debug_msg('python set_ini_path called')
        # boto3-Config aus INI oder config.json lesen
        pass

    def find_first(self, path: str):
        tcbridge.debug_msg('python find_first called')
        yield DirEntry(name="testordner", size=0, is_dir=True)
        yield DirEntry(name="testdatei.txt", size=1234, is_dir=False)
        yield DirEntry(name="noch_eine.bin", size=999999, is_dir=False)

    def get_file(self, remote: str, local: str, flags: int) -> int:
        tcbridge.debug_msg('python get_file called')
        # boto3 download
        # tcbridge.progress(50) für Fortschrittsanzeige
        return 0  # FS_FILE_OK

    def _parse_path(self, path: str):
        pass

    def _list_level(self, bucket: str, prefix: str):
        pass

    def set_ini_path(self, ini_path: str):
        pass

    def put_file(self, local: str, remote: str, flags: int) -> int:
        return 0  # FS_FILE_OK

    def delete_file(self, remote: str) -> bool:
        return False

    def mkdir(self, path: str) -> bool:
        return False
