import shutil
import zipfile
from pathlib import Path

# -----------------------------------------------
# Constants

# Solution file to locate the project root
SOLUTION_NAME = "F0V2.sln"

# Output package name
PACKAGE_DIR_NAME = "Ray99"
PACKAGE_ZIP_NAME = "Ray99.zip"

# Directory names
BUILD_DIR = r"x64/Release"
ASSET_SRC_DIR = r"F0V2/asset"
ASSET_TARGET_DIR = "asset"

# Asset subdirectories to include (keeps their internal directory structure)
ASSET_SUBDIRECTORIES = [
    "course",
    "engine",
    "font",
    "fsr1",
    "image",
    "model",
    "shader",
]


# -----------------------------------------------
# Helper Functions

def find_project_root(start_dir: Path) -> Path:
    """
    Search upward from the current directory until SOLUTION_NAME is found.
    This ensures the script works even when executed from a subfolder.
    """
    cur = start_dir
    while cur != cur.parent:
        if (cur / SOLUTION_NAME).exists():
            return cur
        cur = cur.parent
    raise FileNotFoundError(f"Could not find {SOLUTION_NAME} in any parent directory")


def clean_directory(path: Path):
    """Remove the directory if it exists, then recreate it."""
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def copy_files_with_pattern(src_dir: Path, dst_dir: Path, pattern: str):
    """
    Copy files matching a wildcard pattern (e.g. '*.dll') from src_dir to dst_dir.
    """
    dst_dir.mkdir(parents=True, exist_ok=True)
    for file in src_dir.glob(pattern):
        shutil.copy2(file, dst_dir / file.name)


def copy_asset_subdirectories(project_root: Path, package_root: Path):
    """
    Copy specified asset subdirectories preserving internal directory structure.
    """
    for subdir in ASSET_SUBDIRECTORIES:
        src = project_root / ASSET_SRC_DIR / subdir
        dst = package_root / ASSET_TARGET_DIR / subdir
        if src.exists():
            shutil.copytree(src, dst)
        else:
            print(f"[Warning] Asset directory not found: {src}")


def create_zip_from_directory(src_dir: Path, zip_name: str):
    """
    Create a ZIP archive from a directory.
    """
    with zipfile.ZipFile(zip_name, "w", zipfile.ZIP_DEFLATED) as zf:
        for file_path in src_dir.rglob("*"):
            zf.write(file_path, file_path.relative_to(src_dir))


# -----------------------------------------------
# Main Packaging Process

def main():
    # Step 1: Find project root where F0V2.sln exists
    script_dir = Path.cwd()
    project_root = find_project_root(script_dir)
    print(f"Project root found: {project_root}")

    # Step 2: Prepare output directory
    package_root = project_root / PACKAGE_DIR_NAME
    clean_directory(package_root)

    # Step 3: Copy executable and DLLs from x64/Release
    build_dir = project_root / BUILD_DIR
    if not build_dir.exists():
        raise FileNotFoundError(f"Build directory not found: {build_dir}")

    copy_files_with_pattern(build_dir, package_root, "*.exe")
    copy_files_with_pattern(build_dir, package_root, "*.dll")

    # Step 4: Copy asset directories
    copy_asset_subdirectories(project_root, package_root)

    # Step 5: Zip everything
    zip_path = project_root / PACKAGE_ZIP_NAME
    if zip_path.exists():
        zip_path.unlink()

    create_zip_from_directory(package_root, zip_path)
    print(f"Created: {zip_path}")


if __name__ == "__main__":
    main()
