

[http教学](https://www.runoob.com/http/http-tutorial.html)
[MIME type](https://www.cnblogs.com/jsean/articles/1610265.html)
[CGI](https://www.cnblogs.com/f-ck-need-u/p/7627035.html)





## 基础

### URI和URL
* URI,Uniform Resource Identifier,统一资源标识符
* URL,Uniform Resource Locator,统一资源定位符
* URI用字符串标识某一互联网资源，而URL表示资源的地点，URL是URI的子集


方法+空格+URI+空格+版本+回车+换行
头部字段名+':'+字段值+回车+换行
...
头部字段名+':'+字段值+回车+换行
回车+换行
请求数据(报文主体)



版本+空格+状态码+空格+状态码描述+回车+换行
头部字段名+':'+字段值+回车+换行
...
头部字段名+':'+字段值+回车+换行
回车+换行
报文主体


回车：CR 'r'
换行：LF 'n'
ispace都能检测到它们










## 状态码
* 大类分类

|状态码|类别|描述|
|:---:|:--:|:---:|
|1XX|Information,信息性状态码|接收的请求正在处理|
|2XX|Success，成功状态码|操作被成功接收并处理|
|3XX|Redirection,重定向状态码|需要进一步的操作以完成请求|
|4XX|Client Error,客户端错误|请求包含语法错误或无法完成请求|
|5XX|Server Error, 服务器错误|服务器在处理请求的过程中发生了错误|

* 各小类  

|状态码|英文名 |描述|
|:---:|:----:|:-----:|
|101|Continue|客户端应当继续发送请求|
|102|Switching Protocols|通知客户端采用不同的协议来完成这个请求，只有在切换新的协议更有好处的时候才应该采取类似措施|
|----|----|----|
|200|OK|请求成功，请求所希望的响应头或数据体将随此响应返回|
|201|Created|请求已经被实现，而且有一个新的资源已经依据请求的需要而建立|
|202|Accepted||
|203|||
||||


## https
### http的缺点以及改进方向
* http的不足
    * 通信使用明文，不加密，内容可能会被窃听
    * 不验证通信方的身份，身份有可能遭遇伪装
    * 无法证明报文的完整性，报文有可能已遭篡改
* 加密技术
    * 将通信加密，和SSL(Secure Socket Layer，安全套接层)或TLS(Transport Layer Security,安全传输层协议)组合使用加密通信内容
    * 将内容加密,把HTTP报文里所含的内容进行加密处理，要求客户端和服务器同时具有加密和解密机制，内容仍有被篡改的风险
* 
### https
