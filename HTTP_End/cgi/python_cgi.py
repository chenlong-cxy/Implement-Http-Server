#!/usr/bin/python3

#coding:utf-8

import webbrowser

#命名生成的html
GEN_HTML = "test.html"

#打开文件，准备写入
f = open(GEN_HTML,'w')

#准备相关变量
str1 = 'my name is :'
str2 = '--MichaelAn--'

# 写入HTML界面中
message = """
<html>
<head></head>
<body>
<p>%s</p>
<p>%s</p>
</body>
</html>
"""%(str1,str2)

#写入文件
f.write(message)

#关闭文件
f.close()

#运行完自动在网页中显示
webbrowser.open(GEN_HTML,new = 1)
'''
webbrowser.open(url, new=0, autoraise=True)
Display url using the default browser.
If new is 0, the url is opened in the same browser window if possible.
If new is 1, a new browser window is opened if possible.
If new is 2, a new browser page (“tab”) is opened if possible.
If auto raise is True, the window is raised if possible (note that under many window managers this will occur regardless of the setting of this variable).
'''
