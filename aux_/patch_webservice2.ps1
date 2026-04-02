
$content = Get-Content src\services\WebService.cpp -Raw
$content = $content -replace "``n#include", "`n#include"
Set-Content src\services\WebService.cpp $content

