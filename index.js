var setting = require('./setting.js');
var express = require('express');
var bodyParser = require('body-parser');
var request = require('request');
var app = express();

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

app.set('port', (process.env.PORT || 5000));

app.post('/', function(request, response) {
  console.log('post');
  console.log(request.body);
  response.sendStatus(200);
  twilio();
});

app.get('/', function(request, response) {
  console.log('get');
  console.log(request.body);
  response.sendStatus(200);
});

app.listen(app.get('port'), function() {
  console.log("Node app is running at localhost:" + app.get('port'))
});

function twilio() {
  var headers = {
    'Accept': '*/*',
    'Content-Type': 'application/x-www-form-urlencoded'
  }
  var body = setting.TWILIO_CALL_BODY;
  var url = setting.TWILIO_CALL_URL;
  request({
    url: url,
    method: 'POST',
    headers: headers,
    body: body,
    json: false
  });
}

