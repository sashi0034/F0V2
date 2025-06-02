import os
import sys

# Global variable for the parent root directory
parent_root_dir = "F0V2"


# Function to find the parent root directory
def find_parent_root(start_path):
    """Recursively search for the parent root directory from the given start path."""
    current_path = start_path
    while True:
        if os.path.basename(current_path) == parent_root_dir:
            return current_path
        parent_dir = os.path.dirname(current_path)
        if parent_dir == current_path:  # Reached the top of the file system
            return None
        current_path = parent_dir


# Function to replace text in file names and file contents
def replace_text_in_files(root_dir, old_text, new_text):
    """Recursively replace old_text with new_text in filenames and file contents under root_dir."""
    for dir_path, dir_names, file_names in os.walk(root_dir):
        # Replace in file contents
        for filename in file_names:
            file_path = os.path.join(dir_path, filename)
            try:
                with open(file_path, 'r', encoding='utf-8') as file:
                    file_contents = file.read()
            except UnicodeDecodeError:
                # Skip non-text files
                continue

            if old_text in file_contents:
                new_contents = file_contents.replace(old_text, new_text)
                with open(file_path, 'w', encoding='utf-8') as file:
                    file.write(new_contents)
                print(f"Replaced text in file: {file_path}")

        # Replace in file names
        for filename in file_names:
            if old_text in filename:
                new_filename = filename.replace(old_text, new_text)
                old_file_path = os.path.join(dir_path, filename)
                new_file_path = os.path.join(dir_path, new_filename)
                os.rename(old_file_path, new_file_path)
                print(f"Renamed file: {old_file_path} -> {new_file_path}")


# Main function to get input and trigger replacement
def main():
    # Get the old and new text from standard input
    old_text = input("Enter the text to replace: ")
    new_text = input("Enter the new text: ")

    # Find the starting path and parent root directory
    start_path = os.getcwd()  # Starting from the current working directory
    root_dir = find_parent_root(start_path)

    if root_dir is None:
        print(f"Parent root directory '{parent_root_dir}' not found.")
        sys.exit(1)

    # Perform the text replacement
    replace_text_in_files(root_dir, old_text, new_text)


if __name__ == "__main__":
    main()
