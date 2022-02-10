window.addEventListener('DOMContentLoaded', () => {

    var addon = require('bindings')('uvwork_test');

    const measureEcho = () => {
        let startTime = performance.now();
        return new Promise(resolve => {
            addon.echo('world', () => resolve(performance.now() - startTime));
        });
    }

    const runTest = async () => {
        let elapsed = [];
        for (let i = 0; i < 10; i++) {
            elapsed.push(await measureEcho());
        }
        console.log(`await time: ${elapsed}`);
        const sum = elapsed.reduce((acc, cur) => acc + cur);
        return sum / elapsed.length;
    };

    document.getElementById('test-btn').onclick = () => {
        const element = document.getElementById('test-result-div');
        runTest().then(avgTime => element.innerText = `avg time: ${avgTime}`);
    }

    const replaceText = (selector, text) => {
      const element = document.getElementById(selector)
      if (element) element.innerText = text
    }

    for (const type of ['chrome', 'node', 'electron']) {
      replaceText(`${type}-version`, process.versions[type])
    }
  })
