import * as std from 'std';
import * as os from 'os';
//import * as sb666 from 'sb666';
import * as service from 'service';
import * as logger from 'logger';
//import * as test from 'test';

//service.send()
var console = {}
console.log = value => std.printf(value + "\n");
console.log('ok')
for(var key in globalThis){
    console.log("--->" + key)
}
for(var key in std){
    console.log("--->" + key)
}
for(var key in os){
    console.log("--->" + key)
}
std.printf('hello_world\n')
std.printf(globalThis + "\n")
var a = false
os.setTimeout(()=>{std.printf('AAB\n')}, 2000)
std.printf(a + "\n")

function printStack() {
    try {
        throw new Error("Stack trace");
    } catch (e) {
        const stack = e.stack.split("\n");
        const currentFrame = stack[2]; // 第二行是调用 printStack 的堆栈帧
        console.log(stack); // 打印当前的函数调用堆栈
    }
}

function testFunction() {
    printStack(); // 在这里调用 printStack
}

testFunction();


export async function onMsg(msg_type, msg) {
    logger.info(`onMsg: ${msg_type}, ${msg.value}`);
    let recv_msg = await service.call("TestService", "million.ss.test.LoginReq", {value: "test req"})
    logger.info(`recv_msg ${recv_msg.value}`);
    return ["million.ss.test.LoginRes", {value: "test res"}];
}