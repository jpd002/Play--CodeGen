//Note: MdSubTest and ExternJumpTest still don't work on WebAssembly, 
//they need to be disabled for the whole test suite to run successfully

var factory = require("../../build_cmake/build_wasm/CodeGenTestSuite.js");
factory().then((instance : any) => { 
    console.log("Test complete."); 
});
