import * as std from 'std';
import * as os from 'os';
//import * as sb666 from 'sb666';
import * as service from 'service';

service.send()
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

export async function onMsg(msg) {
    console.log("onMsg called " + msg.value);
    let recv_msg = await service.call("test", "test")
    console.log("recv_msg called " + recv_msg.value);
    return ["sb", {value: 1}];
}