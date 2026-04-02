import re

with open('src/services/WebService.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

new_code = r'''      String page = buildDashboardPage();
      _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      _server.send(200, "text/html", "");
      size_t len = page.length();
      for(size_t i = 0; i < len; i += 512) {
          _server.sendContent(page.substring(i, i + 512));
      }
      _server.sendContent("");
'''

text = re.sub(r'[ \t]*_server\.send\(200,\s*\"text/html\",\s*buildDashboardPage\(\)\);', new_code, text)

with open('src/services/WebService.cpp', 'w', encoding='utf-8') as f:
    f.write(text)

print('Modified handleDashboard successfully.')
