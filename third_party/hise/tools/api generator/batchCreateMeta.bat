@echo off
REM ============================================================================
REM  batchCreateMeta.bat - Full enriched API binary blob pipeline
REM
REM  This is the single entry point for regenerating XmlApi.h/.cpp from the
REM  enriched API data. It supersedes running batchCreate.bat alone, which only
REM  produces Doxygen XML (Step 1 of 6).
REM
REM  Prerequisites: Python 3.x, Doxygen on PATH
REM
REM  Steps:
REM    1. batchCreate.bat           - Run Doxygen, copy/rename XML
REM    2. api_enrich.py phase0      - Parse XML into base JSON
REM    3. api_enrich.py merge       - Merge all enrichment phases
REM    4. api_enrich.py filter-binary - Strip to autocomplete fields
REM    5. ApiValueTreeBuilder.exe   - Emit XmlApi.h + XmlApi.cpp
REM    6. api_enrich.py filter-mcp  - Generate MCP server data
REM ============================================================================

echo === Step 1/6: Running Doxygen XML generation (batchCreate.bat) ===
call batchCreate.bat > NUL 2>&1
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 1 & goto :END )

echo === Step 2/6: Phase 0 - Parsing XML to base JSON ===
python api_enrich.py phase0
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 2 & goto :END )

echo === Step 3/6: Merging all enrichment phases ===
python api_enrich.py merge
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 3 & goto :END )

echo === Step 4/6: Filtering for binary (autocomplete fields only) ===
python api_enrich.py filter-binary
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 4 & goto :END )

echo === Step 5/6: Building C++ binary blob (XmlApi.h + XmlApi.cpp) ===
ApiValueTreeBuilder.exe enrichment\output\filtered_api.json "..\..\hi_scripting\scripting\api" XmlApi
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 5 & goto :END )

echo === Step 6/6: Generating MCP server data ===
python api_enrich.py filter-mcp --output "..\..\tools\mcp_server\data\scripting_api.json"
if %ERRORLEVEL% NEQ 0 ( echo FAILED at Step 6 & goto :END )

echo === Done. XmlApi.h/.cpp updated, MCP server data refreshed ===

:END
