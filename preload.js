window.addEventListener('DOMContentLoaded', (a,b) => {
    const { NativeAddon } = require('bindings')('addon');
    const addon = new NativeAddon();

    const callDispatch = () => {
        let startTime = performance.now();
        return new Promise(resolve => {
            addon.dispatch((duration) => {
                // console.log(`duration: ${duration}`);
                resolve(performance.now() - startTime);
            });
        });
    }

    const runTest = async () => {
        let elapsed = [];
        for (let i = 0; i < 10; i++) {
            elapsed.push(await callDispatch());
        }
        console.log(`await time: ${elapsed}`);
        const sum = elapsed.reduce((acc, cur) => acc + cur);
        return sum / elapsed.length;
    };

    document.getElementById('test-btn').onclick = () => {
        const element = document.getElementById('test-result-div');
        runTest().then(avgTime => element.innerText = `avg time: ${avgTime}`);
    }
  })
