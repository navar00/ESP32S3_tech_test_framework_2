import re

with open('src/services/WebService.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

# Fix handleDashboard
dash_regex = r'_server\.setContentLength\(CONTENT_LENGTH_UNKNOWN\);\s*_server\.send\(200,\s*\"text/html\",\s*\"\"\);\s*size_t\s+len\s*=\s*page\.length\(\);\s*for\(size_t\s+i\s*=\s*0;\s*i\s*<\s*len;\s*i\s*\+=\s*512\)\s*\{\s*_server\.sendContent\(page\.substring\(i,\s*i\s*\+\s*512\)\);\s*\}\s*_server\.sendContent\(\"\"\);'
dash_repl = r'_server.send(200, "text/html", page);'
text = re.sub(dash_regex, dash_repl, text, flags=re.DOTALL)

# Fix handlePalettesGet
pal_regex = r'void WebService::handlePalettesGet\(\)\s*\{.*?_server\.sendContent\(\"\"\);\s*\}'
pal_repl = r'''void WebService::handlePalettesGet() {
    if (!isAuthenticated()) {
        _server.send(401, "text/plain", "Unauthorized");
        return;
    }
    
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "application/json", "");

    size_t len = strlen_P(PALETTES_JSON);
    const size_t CHUNK_SIZE = 1024;
    for (size_t i = 0; i < len; i += CHUNK_SIZE) {
        size_t c_len = (len - i) < CHUNK_SIZE ? (len - i) : CHUNK_SIZE;
        char buf[1025];
        memcpy_P(buf, PALETTES_JSON + i, c_len);
        buf[c_len] = 0;
        _server.sendContent(buf, c_len);
    }
    _server.sendContent("");
}'''
text = re.sub(pal_regex, pal_repl, text, flags=re.DOTALL)

with open('src/services/WebService.cpp', 'w', encoding='utf-8') as f:
    f.write(text)

print('Fixed WebService.cpp')
