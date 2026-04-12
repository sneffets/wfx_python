import tcbridge  # das PYBIND11_EMBEDDED_MODULE von oben

class WfxPlugin:
    """Basisklasse – definiert das Interface"""

    def set_ini_path(self, ini_path: str):
        pass

    def find_first(self, path: str):
        """Muss einen Generator/Iterator zurückgeben der DirEntry-Dicts yieldet"""
        raise NotImplementedError

    def get_file(self, remote: str, local: str, flags: int) -> int:
        raise NotImplementedError

    def put_file(self, local: str, remote: str, flags: int) -> int:
        raise NotImplementedError

    def delete_file(self, remote: str) -> bool:
        raise NotImplementedError

    def mkdir(self, path: str) -> bool:
        raise NotImplementedError


class DirEntry:
    """Was find_first yielden soll"""
    def __init__(self, name: str, size: int, is_dir: bool, mtime=None):
        self.name   = name
        self.size   = size
        self.is_dir = is_dir
        self.mtime  = mtime

