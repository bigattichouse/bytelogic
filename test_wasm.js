const fs = require('fs');

// Mock Math object for WASM imports
const mathImports = {
    Math: {
        sin: Math.sin,
        cos: Math.cos,
        tan: Math.tan,
        log: Math.log,
        pow: Math.pow
    }
};

async function runWasm() {
    try {
        // Load the WASM file
        const wasmBuffer = fs.readFileSync('./examples/example_family.wasm');
        
        // Instantiate the WASM module
        const wasmModule = await WebAssembly.instantiate(wasmBuffer, mathImports);
        
        // Get the exports
        const { main, add_fact, has_fact, memory } = wasmModule.instance.exports;
        
        console.log('✅ WASM module loaded successfully!');
        console.log('Available exports:', Object.keys(wasmModule.instance.exports));
        
        // Run the main function (initializes facts)
        console.log('\n🔄 Running main function...');
        main();
        console.log('✅ Main function completed');
        
        // Test the fact database
        console.log('\n🔍 Testing fact database:');
        
        // Check if alice parent bob (relation_id=6, alice=0, bob=1)
        const hasParentAliceBob = has_fact(6, 0, 1);
        console.log(`has_fact(parent, alice, bob): ${hasParentAliceBob}`);
        
        // Check if bob parent david (relation_id=6, bob=1, david=3) 
        const hasParentBobDavid = has_fact(6, 1, 3);
        console.log(`has_fact(parent, bob, david): ${hasParentBobDavid}`);
        
        // Check a fact that doesn't exist
        const hasParentDavidEve = has_fact(6, 3, 4); // david parent eve - should be false
        console.log(`has_fact(parent, david, eve): ${hasParentDavidEve}`);
        
        console.log('\n✅ WAT->WASM compilation and execution successful!');
        
    } catch (error) {
        console.error('❌ Error running WASM:', error);
        process.exit(1);
    }
}

runWasm();