# Der Kernwitz: find_first ist ein Python-Generator.
# C++ speichert das Generator-Objekt pro Handle in der Map und ruft einfach
# __next__() auf – damit ist das parallele-Panels-Problem elegant gelöst
# ohne irgendwelchen globalen State auf Python-Seite. Jedes Handle hat seinen
# eigenen Generator mit eigenem Zustand.

import tcbridge  # das PYBIND11_EMBEDDED_MODULE von oben

from datetime import datetime

# Statischer Testbaum
TREE = {
    "\\": {
        "dirs":  ["Musik", "Dokumente", "Bilder"],
        "files": []
    },
    "\\Musik\\": {
        "dirs":  ["Rock", "Jazz"],
        "files": [
            ("playlist.m3u", 2048),
            ("favorites.m3u", 512),
        ]
    },
    "\\Musik\\Rock\\": {
        "dirs":  [],
        "files": [
            ("song1.mp3", 4_500_000),
            ("song2.mp3", 3_800_000),
        ]
    },
    "\\Musik\\Jazz\\": {
        "dirs":  [],
        "files": [
            ("coltrane.flac", 28_000_000),
            ("miles.flac",    31_000_000),
        ]
    },
    "\\Dokumente\\": {
        "dirs":  ["Vertraege", "Rechnungen"],
        "files": [
            ("readme.txt", 1024),
        ]
    },
    "\\Dokumente\\Vertraege\\": {
        "dirs":  [],
        "files": [
            ("mietvertrag.pdf",  102_400),
            ("arbeitsvertrag.pdf", 98_304),
        ]
    },
    "\\Dokumente\\Rechnungen\\": {
        "dirs":  [],
        "files": [
            ("januar.pdf",  20_480),
            ("februar.pdf", 18_432),
        ]
    },
    "\\Bilder\\": {
        "dirs":  ["Urlaub"],
        "files": [
            ("wallpaper.png", 2_097_152),
        ]
    },
    "\\Bilder\\Urlaub\\": {
        "dirs":  [],
        "files": [
            ("strand.jpg",   3_145_728),
            ("berge.jpg",    2_621_440),
            ("stadtplan.jpg",  524_288),
        ]
    },
}

MTIME = datetime(2024, 6, 15, 12, 0, 0)

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

class S3Plugin(WfxPlugin):

    def _normalize(self, path: str) -> str:
        """TC liefert Pfade mit / oder \\ und manchmal ohne trailing \\ """
        path = path.replace("/", "\\")
        if not path.endswith("\\"):
            path += "\\"
        # Sicherstellen dass er mit \\ beginnt
        if not path.startswith("\\"):
            path = "\\" + path
        return path

    def find_first(self, path: str):
        norm = self._normalize(path)
        node = TREE.get(norm)

        if node is None:
            return  # leerer Generator → INVALID_HANDLE_VALUE

        for dirname in node["dirs"]:
            yield DirEntry(
                name   = dirname,
                size   = 0,
                is_dir = True,
                mtime  = MTIME
            )

        for filename, size in node["files"]:
            yield DirEntry(
                name   = filename,
                size   = size,
                is_dir = False,
                mtime  = MTIME
            )

    def get_file(self, remote: str, local: str, flags: int) -> int:
        # Für den Test einfach eine leere Datei anlegen
        import tcbridge
        tcbridge.progress(remote, local, 0)
        with open(local, "wb") as f:
            f.write(b"TEST CONTENT: " + remote.encode())
        tcbridge.progress(remote, local, 100)
        return 0  # FS_FILE_OK

    def put_file(self, local: str, remote: str, flags: int) -> int:
        return 0

    def delete_file(self, remote: str) -> bool:
        return False  # readonly Testplugin

    def mkdir(self, path: str) -> bool:
        return False

class S3Plugin_old(WfxPlugin):

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
