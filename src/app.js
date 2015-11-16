Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://spielzeugland.github.io/pebble_blocks/config/';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var backgroundColor = configData['background_color'];
	var foregroundColor = configData['foreground_color'];
	var blockSize = configData['block_size'];
  var blockStyleString = configData['block_style'];
	var blockStyle = 0;
	if(blockStyleString === "block") {
		blockStyle = 0;
	} else if (blockStyleString === "circle") {
		blockStyle = 1;
	}
	
  var dict = {};
  dict['BG_RED'] = parseInt(backgroundColor.substring(2, 4), 16);
  dict['BG_GREEN'] = parseInt(backgroundColor.substring(4, 6), 16);
  dict['BG_BLUE'] = parseInt(backgroundColor.substring(6), 16);
  dict['FG_RED'] = parseInt(foregroundColor.substring(2, 4), 16);
  dict['FG_GREEN'] = parseInt(foregroundColor.substring(4, 6), 16);
  dict['FG_BLUE'] = parseInt(foregroundColor.substring(6), 16);
  dict['BLOCK_STYLE'] = blockStyle;
	dict['BLOCK_SIZE'] = parseInt(blockSize, 10);
	
  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});