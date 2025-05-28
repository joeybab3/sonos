var volume = 0;
var lastResponse = "";

async function sendCmd(cmd) {
    try {
        const response = await fetch(`/cmd?cmd=${cmd}`);
        if (response.ok) {
            const text = await response.text();
            addResponse(text + "<br/>");
        }
    } catch (error) {
        addResponse(`Error: ${error}<br/>`);
    }
}

function parseCmd(cmd) {
    document.getElementById('console_text').value = '';
    sendCmd(cmd);
}

async function setVolume(vol) {
    vol = parseInt(vol);
    if(Number.isInteger(vol)) {
        if(vol > 99 || vol < 0) {
            addResponse("Your number must be between 0 and 99<br/>");
            document.getElementById("volume").value = volume;
        } else {
            await sendCmd(vol);
            volume = vol;
            await getVolume();
        }
    } else {
        addResponse("You must enter a number<br/>");
        document.getElementById("volume").value = volume;
    }
}

function addResponse(response) {
    document.getElementById("response").innerHTML += response;
    var elem = document.getElementById('response');
      elem.scrollTop = elem.scrollHeight;    
}

async function getVolume() {
    try {
        const response = await fetch("/get?cmd=gv");
        if (response.ok) {
            const text = await response.text();
            addResponse("v = "+text+"<br/>");
            volume = text;
            document.getElementById("volume").value = volume;
            document.getElementById("vol").textContent = volume;
            document.getElementById("volume-slider").value = volume;
        }
    } catch (error) {
        addResponse(`Error: ${error}<br/>`);
    }
}

async function getTrack() {
    const songType = await sendReq("gs");
    addResponse("SongType = "+songType+"<br/>");
}

async function sendReq(cmd) {
    try {
        const response = await fetch(`/get?cmd=${cmd}`);
        if (response.ok) {
            lastResponse = await response.text();
            return lastResponse;
        }
    } catch (error) {
        addResponse(`Error: ${error}<br/>`);
    }
    return lastResponse;
}

async function updateStuff() {
    await getTrack();
    await getVolume();    
}
