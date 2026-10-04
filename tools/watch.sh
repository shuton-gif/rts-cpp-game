#!/bin/sh
# rebuilds build/rts every time a source, sprite or CMakeLists.txt changes; Ctrl+C stops

cd "$(dirname "$0")/.." || exit 1

if ! command -v fswatch >/dev/null 2>&1; then
    echo "fswatch not found: brew install fswatch"
    exit 1
fi

if [ ! -f build/CMakeCache.txt ]; then
    cmake -S . -B build || exit 1
fi

build() {
    if cmake --build build; then
        echo "✓ build ok $(date +%H:%M:%S)"
    else
        echo "✗ build failed $(date +%H:%M:%S)"
    fi
}

build
echo "watching src/ assets/ CMakeLists.txt"

fswatch -o -l 0.3 -E -e '.*' -i '\.(cpp|h|hpp|png)$' -i 'CMakeLists\.txt$' \
    --event Created --event Updated --event Removed --event Renamed --event AttributeModified \
    src assets CMakeLists.txt |
while read -r _; do
    build
done
# build: every code edit needed a manual cmake --build like next.js without --watch; fswatch batches each burst of saves into one event and the loop rebuilds and prints ✓ or ✗ with the time, configuring build/ first when it does not exist; only content events count, because the post-build copy reading assets/ raises PlatformSpecific events that would loop into extra rebuilds
