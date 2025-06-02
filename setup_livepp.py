import sys
import re

# Get file path
if len(sys.argv) >= 2:
    path = sys.argv[1]
else:
    path = input("Enter path to .vcxproj file: ").strip()

with open(path, "r", encoding="utf-8") as f:
    content = f.read()

# Match Debug|x64 <ItemDefinitionGroup>
pattern = r'(<ItemDefinitionGroup\s+Condition="\'\$\((Configuration)\)\|\$\((Platform)\)\'==\'Debug\|x64\'">.*?</ItemDefinitionGroup>)'
match = re.search(pattern, content, re.DOTALL)
if not match:
    print("Target configuration block not found.")
    sys.exit(1)

block = match.group(1)

# Fixed indentation
fixed_indent = "      "

# Tags to insert or update
cl_tags = [
    ("DebugInformationFormat", "OldStyle"),
    ("MinimalRebuild", "false"),
    ("CreateHotpatchableImage", "true"),
]
link_tags = [
    ("GenerateDebugInformation", "DebugFull"),
    ("CreateHotPatchableImage", "Enabled"),
    ("OptimizeReferences", "false"),
    ("EnableCOMDATFolding", "false"),
]

# Update or insert tags in a given parent block
def update_or_insert_tags(block, parent_tag, new_tags):
    parent_pattern = rf'(<{parent_tag}>.*?</{parent_tag}>)'
    match = re.search(parent_pattern, block, re.DOTALL)
    if match:
        lines = match.group(1).splitlines()
        # Remove lines with matching tags
        lines = [line for line in lines if not any(f"<{tag}>" in line for tag, _ in new_tags)]
        # Insert new tags before closing tag
        for tag, value in new_tags:
            lines.insert(-1, f"{fixed_indent}<{tag}>{value}</{tag}>")
        new_parent = "\n".join(lines)
        return block.replace(match.group(1), new_parent)
    else:
        # Create new block and insert
        new_block = [f"{fixed_indent}<{parent_tag}>"]
        for tag, value in new_tags:
            new_block.append(f"{fixed_indent}<{tag}>{value}</{tag}>")
        new_block.append(f"{fixed_indent}</{parent_tag}>")
        return block.replace('</ItemDefinitionGroup>', "\n" + "\n".join(new_block) + '\n    </ItemDefinitionGroup>')

# Update block
block = update_or_insert_tags(block, "ClCompile", cl_tags)
block = update_or_insert_tags(block, "Link", link_tags)

# Replace in original content
new_content = content.replace(match.group(1), block)

# Write back to file
with open(path, "w", encoding="utf-8") as f:
    f.write(new_content)

print("Successfully updated Debug|x64 block.")
