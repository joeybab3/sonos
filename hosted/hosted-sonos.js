var volume = 0;
var lastResponse = "";

function sendCmd(cmd)
{
	var xhttp = new XMLHttpRequest();
  	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  addResponse(this.responseText+"<br/>");
		}
  	};
	var url = "/cmd?cmd=" + cmd;
  	xhttp.open("GET", url, true);
  	xhttp.send();
}

function parseCmd(cmd)
{
	$('#console_text').val('');
	sendCmd(cmd);
}

function setVolume(vol)
{
	vol = parseInt(vol);
	if(Number.isInteger(vol))
	{
		if(vol > 99 || vol < 0)
		{
			addResponse("Your number must be between 0 and 99<br/>");
			document.getElementById("volume").value = volume;
		}
		else
		{
			sendCmd(vol);
			volume = vol;
			getVolume();
		}
	}
	else
	{
		addResponse("You must enter a number<br/>");
		document.getElementById("volume").value = volume;
	}
}

function addResponse(response)
{
	document.getElementById("response").innerHTML += response;
	var elem = document.getElementById('response');
  	elem.scrollTop = elem.scrollHeight;	
}

function getVolume()
{
	var xhttp = new XMLHttpRequest();
  	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  addResponse("v = "+this.responseText+"<br/>");
		  volume = this.responseText;
		  $("#volume").val(volume);
		  $("#vol").text(volume);
		  $("#volume-slider").val(volume)
		}
  	};
	var url = "/get?cmd=gv";
  	xhttp.open("GET", url, true);
  	xhttp.send();
}

function getTrack()
{
	addResponse("SongType = "+sendReq("gs")+"<br/>");
}

function sendReq(cmd)
{
	var xhttp = new XMLHttpRequest();
  	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
		  lastResponse = this.responseText;
		}
  	};
	var url = "/get?cmd=" + cmd;
  	xhttp.open("GET", url, true);
  	xhttp.send();
	return lastResponse;
}

function updateStuff()
{
	getTrack();
	getVolume();	
}

function changeIp(ip)
{
	//sendCmd(	
}