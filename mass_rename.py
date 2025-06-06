import os
import sys

# Global variable for the parent root directory name
parent_root_dir = "F0V2"

# Set of directory names to exclude from processing
excluded_dirs = {".git", ".vs", "__pycache__"}  # Add more as needed


def find_parent_root(start_path):
    """Recursively search for the parent root directory from the given start path."""
    current_path = start_path
    while True:
        if os.path.basename(current_path) == parent_root_dir:
            return current_path
        parent_dir = os.path.dirname(current_path)
        if parent_dir == current_path:  # Reached the root of the filesystem
            return None
        current_path = parent_dir


def replace_text_in_files(root_dir, old_text, new_text):
    """Recursively replace old_text with new_text in file names and file contents under root_dir."""
    for dir_path, dir_names, file_names in os.walk(root_dir):
        # Exclude specified directories from traversal
        dir_names[:] = [d for d in dir_names if d not in excluded_dirs]

        # Replace text in file contents
        for filename in file_names:
            file_path = os.path.join(dir_path, filename)
            try:
                with open(file_path, "r", encoding="utf-8") as file:
                    file_contents = file.read()
            except UnicodeDecodeError:
                # Skip non-text or binary files
                continue

            if old_text in file_contents:
                new_contents = file_contents.replace(old_text, new_text)
                with open(file_path, "w", encoding="utf-8") as file:
                    file.write(new_contents)
                print(f"Replaced text in file: {file_path}")

        # Replace text in file names
        for filename in file_names:
            if old_text in filename:
                new_filename = filename.replace(old_text, new_text)
                old_file_path = os.path.join(dir_path, filename)
                new_file_path = os.path.join(dir_path, new_filename)
                os.rename(old_file_path, new_file_path)
                print(f"Renamed file: {old_file_path} -> {new_file_path}")


def main():
    # Get input strings from the user
    old_text = input("Enter the text to replace: ")
    new_text = input("Enter the new text: ")

    # Start from the current working directory and find the parent root
    start_path = os.getcwd()
    root_dir = find_parent_root(start_path)

    if root_dir is None:
        print(f"Parent root directory '{parent_root_dir}' not found.")
        sys.exit(1)

    # Perform replacements
    replace_text_in_files(root_dir, old_text, new_text)


if __name__ == "__main__":
    main()
