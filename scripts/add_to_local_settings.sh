path="$1"

mkdir -p .vscode

if [[ -f .vscode/settings.json ]]; then
    jq --arg path "$path" \
        '.["clangd.path"] = $path' \
        .vscode/settings.json > .vscode/settings.json.tmp &&
    mv .vscode/settings.json.tmp .vscode/settings.json
else
    jq -n --arg path "$path" \
        '{ "clangd.path": $path }' \
        > .vscode/settings.json
fi