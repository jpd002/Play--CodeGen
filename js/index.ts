//Note: MdSubTest and ExternJumpTest still don't work on WebAssembly, 
//they need to be disabled for the whole test suite to run successfully

import path from "path";

var codeGenTestSuiteModulePath = path.join(process.cwd() + "/CodeGenTestSuite.js");
var factory = require(codeGenTestSuiteModulePath);
factory().then((instance : any) => {
    console.log("Test complete."); 
});
