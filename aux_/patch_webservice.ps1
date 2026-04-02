
$content = Get-Content src\services\WebService.cpp -Raw
$content = $content -replace '_server.on\("/favicon.ico", HTTP_GET, \[this\]\(\)\s*\{ handleFavicon\(\); \}\);', "_server.on(""/api/palettes"", HTTP_GET, [this](){ handlePalettesGet(); });`n    _server.on(""/api/palettes/set"", HTTP_POST, [this](){ handlePaletteSet(); });`n    _server.on(""/favicon.ico"", HTTP_GET, [this](){ handleFavicon(); });"

$content = $content -replace '#include "../core/GOLConfig.h"', '#include "../core/GOLConfig.h"`n#include "../core/PalettesData.h"'

Set-Content src\services\WebService.cpp $content

