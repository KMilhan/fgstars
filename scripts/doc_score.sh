#!/usr/bin/env bash
set -euo pipefail

DOCS=(README* *.md *.dox)

files=()
for f in "${DOCS[@]}"; do
  if [ -f "$f" ]; then
    if ! [[ " ${files[*]} " == *" $f "* ]]; then
      files+=("$f")
    fi
  fi
done

if [ ${#files[@]} -eq 0 ]; then
  echo '{"overall":0,"llm_average":0,"details":{"error":"No docs found"}}'
  exit 1
fi

relevance_patterns=(
  '\bqt[[:space:]]*5(?:\.[0-9]+)?\+?\b'
  '\bkf5\b'
  '\bkde4\b'
  '\bSVN\b'
  'Konqueror'
  'Phabricator'
)

relevance_count=0
for f in "${files[@]}"; do
  for p in "${relevance_patterns[@]}"; do
    count=$(rg -o -i -N "$p" "$f" | wc -l || true)
    relevance_count=$((relevance_count + count))
  done
done

typos=(
  'avaialble'
  'writtetn'
  'maintain ined'
  'recommened'
)

typo_count=0
for f in "${files[@]}"; do
  for p in "${typos[@]}"; do
    count=$(rg -o -i -N "$p" "$f" | wc -l || true)
    typo_count=$((typo_count + count))
  done
done

# Detect reversed markdown links: (text)[url]
bad_links=$(rg -o -N '\\([^)]*\\)\\[[^]]+\\]' "${files[@]}" | wc -l || true)

overall=$((100 - relevance_count*8 - typo_count*2 - bad_links*12))
llm_average=$((100 - relevance_count*10 - typo_count*2 - bad_links*15))

if [ "$overall" -lt 0 ]; then
  overall=0
fi
if [ "$llm_average" -lt 0 ]; then
  llm_average=0
fi
if [ "$overall" -gt 100 ]; then
  overall=100
fi
if [ "$llm_average" -gt 100 ]; then
  llm_average=100
fi

cat <<JSON
{
  "overall": $overall,
  "llm_average": $llm_average,
  "details": {
    "docs": ${#files[@]},
    "relevance_issues": $relevance_count,
    "typos": $typo_count,
    "bad_markdown_links": $bad_links
  }
}
JSON
