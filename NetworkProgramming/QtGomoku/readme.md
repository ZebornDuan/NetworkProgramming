# QtGomoku

基于Qt的网络五子棋游戏，采用客户端与服务器一体化设计，既可作为服务器创建连接也可作为客户端申请向目标IP地址建立连接。连接基于TCP/IP协议。服务器在完成创建连接的操作后会以本机IP地址选择适当的端口进入监听状态，一旦有申请向本机IP建立的请求即可建立连接。客户端则会根据用户输入的IP地址向目标IP地址发出连接申请，若目标IP地址正处于监听状态则会成功建立连接，否则若三秒内无法建立连接则会提示“连接失败“，用户应检测目标IP地址的合法性与或者确定对方的状态。建立连接后双方会通过TCP 套接字实现通信以更新棋盘与相应的对局互动操作。用户在运行程序后可根据程序界面上的按钮的内容来完成相应的操作。提示功能可以在轮到我方落子时给出相应的提示。提示内容包括对方在某位置再落一子则我方必败 （红色标记）或者我方在某位置再落一子则我方必胜 （绿色标记）的情形。

![preview](https://github.com/ZebornDuan/ComputerNetworks/blob/master/NetworkProgramming/chatting/server/download/3.png)