//Note: MdSubTest and ExternJumpTest still don't work on WebAssembly, 
//they need to be disabled for the whole test suite to run successfully

//If running this in VS code, make sure you are debugging and that you are
//breaking on caught exceptions to get as much info as possible.

import path from "path";

var codeGenTestSuiteModulePath = path.join(process.cwd() + "/CodeGenTestSuite.js");
var factory = require(codeGenTestSuiteModulePath);
factory().then((instance : any) => {
    if(process.exitCode != 0) {
        console.log("Tests failed.");
    } else {
        console.log("Tests succeeded."); 
    }
});
