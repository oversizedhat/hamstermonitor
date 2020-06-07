var http = require("http");

const sendData = (lapTime, lapCount) => {
  const postData = `hamster,host=poppe lapTime=${lapTime},lapCount=${lapCount}`; 
 
  const options = {
    host: '192.168.0.204',
    port: 8186,
    path: '/write?db=influx&precision=ms',
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'}
  };

  const req = http.request(options, (res) => {
    console.log(`STATUS: ${res.statusCode}`);
    console.log(`HEADERS: ${JSON.stringify(res.headers)}`);
    res.setEncoding('utf8');
    res.on('data', (chunk) => {
      console.log(`BODY: ${chunk}`);
    });

    res.on('end', () => {
      console.log('No more data in response.');
    });
  });

  req.on('error', (e) => {
    console.log(e);
    console.error(`problem with request: ${e.message}`);
  });

  req.write(postData);
  req.end();
};

let counter = 0;
const sendMockMetrics = async () => {
   counter++;
    sendData(1000 + 200*Math.random(), counter);

    //keep sending forever
    setTimeout(sendMockMetrics, 1000);
}

sendMockMetrics();