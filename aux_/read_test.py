import sys

p = r"c:\Users\egavi\OneDrive\Documents\PlatformIO\Projects\ESP32S3_tech_test_framework_2\src\services\WebService.cpp"
try:
    with open(p, "r", encoding="utf-8") as f:
        content = f.read()
    print("FILE SIZE:", len(content))
    idx = content.find('buildDashboardPage')
    print("Found at:", idx)
    if idx != -1:
        print("CONTENT SNIPPET:")
        print(content[max(0, idx-500) : idx+2000])
except Exception as e:
    print("ERROR:", e)
