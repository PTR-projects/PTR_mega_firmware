var getDataIntervalID;
var getDataLiveIntervalID;


function SelectSection_Home() {
	window.scrollTo(0, 0);
	TabsSelect(1);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');
	
	main_tab.style.display		= 'block';
	storage_tab.style.display 	= 'none';
	ign_tab.style.display		= 'none';
	settings_tab.style.display	= 'none';
	liveview_tab.style.display	= 'none';

	clearInterval(getDataLiveIntervalID);
}

function SelectSection_Storage() {
	window.scrollTo(0, 0);
	TabsSelect(2);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');
	
	main_tab.style.display		= 'none';
	storage_tab.style.display	= 'block';
	ign_tab.style.display		= 'none';
	settings_tab.style.display	= 'none';
	liveview_tab.style.display	= 'none';

	clearInterval(getDataLiveIntervalID);
}

function SelectSection_Ign() {
	window.scrollTo(0, 0);
	TabsSelect(3);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');
	
	main_tab.style.display		= 'none';
	storage_tab.style.display	= 'none';
	ign_tab.style.display		= 'block';
	settings_tab.style.display	= 'none';
	liveview_tab.style.display	= 'none';

	clearInterval(getDataLiveIntervalID);
}

function SelectSection_Settings() {
	window.scrollTo(0, 0);
	TabsSelect(4);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');
	
	main_tab.style.display		= 'none';
	storage_tab.style.display	= 'none';
	ign_tab.style.display		= 'none';
	settings_tab.style.display	= 'block';
	liveview_tab.style.display	= 'none';

	clearInterval(getDataLiveIntervalID);
}

function SelectSection_Live() {
	window.scrollTo(0, 0);
	TabsSelect(5);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');
	
	main_tab.style.display		= 'none';
	storage_tab.style.display	= 'none';
	ign_tab.style.display		= 'none';
	settings_tab.style.display	= 'none';
	liveview_tab.style.display	= 'block';

	getDataLiveIntervalID = setInterval(getDataLive, 1000);
}

function TabsInit() {
	console.log("Tabs init");
	window.scrollTo(0, 0);
	TabsSelect(1);
	var main_tab     = document.getElementById('tab-main');
	var storage_tab  = document.getElementById('tab-storage');
	var ign_tab      = document.getElementById('tab-igniters');
	var settings_tab = document.getElementById('tab-settings');
	var liveview_tab = document.getElementById('tab-live');

	main_tab.style.display		= 'block';
	storage_tab.style.display	= 'none';
	ign_tab.style.display		= 'none';
	settings_tab.style.display	= 'none';
	liveview_tab.style.display	= 'none';

	clearInterval(getDataLiveIntervalID);
}

function TabsSelect(num){
	var home 		= document.getElementById('menu_home');
	var storage 	= document.getElementById('menu_storage');
	var igniters 	= document.getElementById('menu_ign');
	var settings 	= document.getElementById('menu_settings');
	var live 		= document.getElementById('menu_live');
	
	var home_td 	= document.getElementById('menu_home_td');
	var storage_td 	= document.getElementById('menu_storage_td');
	var igniters_td = document.getElementById('menu_ign_td');
	var settings_td = document.getElementById('menu_settings_td');
	var live_td 	= document.getElementById('menu_live_td');
	
	switch(num){
		case 0:
			home.style.color 			= '#f2f2f2';
			home.style.background 		= '#333';
			home_td.style.background 	= '#333';
			storage.style.color 		= '#f2f2f2';
			storage.style.background 	= '#333';
			storage_td.style.background = '#333';
			igniters.style.color 		= '#f2f2f2';
			igniters.style.background 	= '#333';
			igniters_td.style.background= '#333';
			settings.style.color 		= '#f2f2f2';
			settings.style.background 	= '#333';
			settings_td.style.background= '#333';
			live.style.color 			= '#f2f2f2';
			live.style.background 		= '#333';
			live_td.style.background 	= '#333';
			break;
		
		case 1:
			home.style.color 			= 'black';
			home.style.background 		= '#ddd';
			home_td.style.background 	= '#ddd';
			storage.style.color 		= '#f2f2f2';
			storage.style.background 	= '#333';
			storage_td.style.background = '#333';
			igniters.style.color 		= '#f2f2f2';
			igniters.style.background 	= '#333';
			igniters_td.style.background= '#333';
			settings.style.color 		= '#f2f2f2';
			settings.style.background 	= '#333';
			settings_td.style.background= '#333';
			live.style.color 			= '#f2f2f2';
			live.style.background 		= '#333';
			live_td.style.background 	= '#333';
			break;
			
		case 2:
			home.style.color 			= '#f2f2f2';
			home.style.background 		= '#333';
			home_td.style.background 	= '#333';
			storage.style.color 		= 'black';
			storage.style.background 	= '#ddd';
			storage_td.style.background = '#ddd';
			igniters.style.color 		= '#f2f2f2';
			igniters.style.background 	= '#333';
			igniters_td.style.background= '#333';
			settings.style.color 		= '#f2f2f2';
			settings.style.background 	= '#333';
			settings_td.style.background= '#333';
			live.style.color 			= '#f2f2f2';
			live.style.background 		= '#333';
			live_td.style.background 	= '#333';
			break;
			
		case 3:
			home.style.color 			= '#f2f2f2';
			home.style.background 		= '#333';
			home_td.style.background 	= '#333';
			storage.style.color 		= '#f2f2f2';
			storage.style.background 	= '#333';
			storage_td.style.background = '#333';
			igniters.style.color 		= 'black';
			igniters.style.background 	= '#ddd';
			igniters_td.style.background= '#ddd';
			settings.style.color 		= '#f2f2f2';
			settings.style.background 	= '#333';
			settings_td.style.background= '#333';
			live.style.color 			= '#f2f2f2';
			live.style.background 		= '#333';
			live_td.style.background 	= '#333';
			break;
			
		case 4:
			home.style.color 			= '#f2f2f2';
			home.style.background 		= '#333';
			home_td.style.background 	= '#333';
			storage.style.color 		= '#f2f2f2';
			storage.style.background 	= '#333';
			storage_td.style.background = '#333';
			igniters.style.color 		= '#f2f2f2';
			igniters.style.background 	= '#333';
			igniters_td.style.background= '#333';
			settings.style.color 		= 'black';
			settings.style.background 	= '#ddd';
			settings_td.style.background= '#ddd';
			live.style.color 			= '#f2f2f2';
			live.style.background 		= '#333';
			live_td.style.background 	= '#333';
			break;
			
		case 5:
			home.style.color 			= '#f2f2f2';
			home.style.background 		= '#333';
			home_td.style.background 	= '#333';
			storage.style.color 		= '#f2f2f2';
			storage.style.background 	= '#333';
			storage_td.style.background = '#333';
			igniters.style.color 		= '#f2f2f2';
			igniters.style.background 	= '#333';
			igniters_td.style.background= '#333';
			settings.style.color 		= '#f2f2f2';
			settings.style.background 	= '#333';
			settings_td.style.background= '#333';
			live.style.color 			= 'black';
			live.style.background 		= '#ddd';
			live_td.style.background 	= '#ddd';
			break;
	}
}

/*-------------------- Storage tab -------------------------------*/
function storage_download_handler () {
	console.log("Storage - Download pressed");
	location.href = '/storage/meas.bin';
}

function storage_remove_handler () {
	console.log("Storage - Remove pressed");
	if(confirm('Are you sure?')) { 
		POST_simple("/delete/storage/meas.bin", '');		
	}
}

/*--------------------- Igniters tab ------------------------------*/
function ign_ign1_unlock_handler () {
	navigator.vibrate(200); 
	if(confirm('Are you sure? Igniter 1 will be unlocked!')) {
		document.getElementById('button-ign1-unlock').disabled = true;   
		document.getElementById('button-ign1-fire').disabled = false;                 
		document.getElementById('label-igniter1-lock-status').textContent = "Unlocked!";
		document.getElementById('label-igniter1-lock-status').style.color = "red";
	}
}

function ign_ign2_unlock_handler () {
	navigator.vibrate(200); 
	if(confirm('Are you sure? Igniter 2 will be unlocked!')) {
		document.getElementById('button-ign2-unlock').disabled = true;   
		document.getElementById('button-ign2-fire').disabled = false;                 
		document.getElementById('label-igniter2-lock-status').textContent = "Unlocked!";
		document.getElementById('label-igniter2-lock-status').style.color = "red";
	}
}

function ign_ign3_unlock_handler ()  {
	navigator.vibrate(200); 
	if(confirm('Are you sure? Igniter 3 will be unlocked!')) {
		document.getElementById('button-ign3-unlock').disabled = true;   
		document.getElementById('button-ign3-fire').disabled = false;                 
		document.getElementById('label-igniter3-lock-status').textContent = "Unlocked!";
		document.getElementById('label-igniter3-lock-status').style.color = "red";
	}
}

function ign_ign4_unlock_handler ()  {
	navigator.vibrate(200); 
	if(confirm('Are you sure? Igniter 4 will be unlocked!')) {
		document.getElementById('button-ign4-unlock').disabled = true;   
		document.getElementById('button-ign4-fire').disabled = false;                 
		document.getElementById('label-igniter4-lock-status').textContent = "Unlocked!";
		document.getElementById('label-igniter4-lock-status').style.color = "red";
	}
}

function ign_ign1_fire_handler() {
	console.log("Igniters - Fire ign 1");
	navigator.vibrate(200); 
	POST_simple("cmd", 'fire-ign-1=8342');
}

function ign_ign2_fire_handler() {
	console.log("Igniters - Fire ign 2");
	navigator.vibrate(200); 
	POST_simple("cmd", 'fire-ign-2=8464');
}

function ign_ign3_fire_handler() {
	console.log("Igniters - Fire ign 3");
	navigator.vibrate(200); 
	POST_simple("cmd", 'fire-ign-3=1523');
}

function ign_ign4_fire_handler() {
	console.log("Igniters - Fire ign 4");
	navigator.vibrate(200); 
	POST_simple("cmd", 'fire-ign-4=6123');
}

function POST_simple(url, data) {
  return fetch(url, {method: "POST", headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: data});
}

/* Function to retrieve data from the remote server*/
function getData() {
  fetch("/status")
	  .then(response => {
		if (response.ok) {
		  /* The connection was successful*/
		  return response.json();
		} else {
		  /* The connection was not successful*/
		  throw new Error(`Error ${response.status}: ${response.statusText}`);
		}
	  })
	  .then(data => {
			/* Update the text label with the new data*/
			console.log("Refresh Status OK");
			document.getElementById("label-status-serial-number").textContent 	= data.configuration.serialNumber;
			document.getElementById("label-status-software-version").textContent 	= data.configuration.softwareVersion;
			document.getElementById("label-status-system-state").textContent 	= data.logic.state;
			document.getElementById("label-status-timestamp").textContent 	= data.logic.timestamp;
			document.getElementById("label-status-storage").textContent 	= data.sysMgr.Storage_driver;
			document.getElementById("label-status-angle").textContent 	= data.sensors.angle;
			document.getElementById("label-status-latitude").textContent 	= data.sensors.gps.latitude.direction + data.sensors.gps.latitude.value;
			document.getElementById("label-status-longitude").textContent 	= data.sensors.gps.longitude.direction + data.sensors.gps.latitude.value;
	  })
	  .catch(error => {
		/* Handle any errors that occurred*/
		console.error(error);
	  });
}

function webInit(){
	TabsInit();
	/* Set an interval to retrieve new data every 10 second*/
	setInterval(getData, 10000);
	initDataLive();
}

function createLiveTable(json) {
    if (typeof json === 'undefined') {
        console.log("JSON does not exist!");
        return;
    }

    var sensors_num = Object.entries(json).length;
    if (sensors_num == 0) {
        console.log("JSON length equal to zero!");
        return;
    }

    var json_objects = Object.entries(json);

    const body = document.getElementById('tab-live');
    const tbl = document.createElement('table');
    tbl.className = "table-live";

    /*tbl.style.width = '100px';*/
    /*tbl.style.border = '1px solid black';*/

    for (let i = 0; i < sensors_num; i++) {
        if (i > 0) {
            const tr = tbl.insertRow();
            const td = tr.insertCell();
            /*td.style.border = '1px solid black';*/
            const td2 = tr.insertCell();
            /*td2.style.border = '1px solid black';*/
			tr.style.backgroundColor = '#f2f2f2';
        }
        const tr = tbl.insertRow();
        const td = tr.insertCell();
        td.appendChild(document.createTextNode(json_objects[i][0]));
        td.style.fontWeight = 'bold';

        var object_entries = Object.entries(json_objects[i][1]);

        for (let j = 0; j < object_entries.length; j++) {
            let tr2 = tbl.insertRow();
            const td2 = tr2.insertCell();
            td2.appendChild(document.createTextNode(object_entries[j][0]));
            /*td2.style.border = '1px solid black';*/

            const td3 = tr2.insertCell();
            var element_value = document.createElement('label');
            element_value.id = json_objects[i][0].replace(/\s/g, '_') + '-' + object_entries[j][0].replace(/\s/g, '_') + '-value';
            if (typeof object_entries[j][1] == 'number') {
                element_value.innerHTML = Number.parseFloat(object_entries[j][1]).toPrecision(6);
            } else if(typeof object_entries[j][1] == 'string'){
				element_value.innerHTML = object_entries[j][1];
			} else {
                element_value.innerHTML = 'NaN';
            }
            td3.appendChild(element_value);
            /*td3.style.border = '1px solid black';*/
        }
    }

    body.appendChild(tbl);
}

function updateLiveTable(json) {
    if (typeof json === 'undefined') {
        console.log("JSON does not exist!");
        return;
    }

    var sensors_num = Object.entries(json).length;
    if (sensors_num == 0) {
        console.log("JSON length equal to zero!");
        return;
    }

    var json_objects = Object.entries(json);

    for (let i = 0; i < sensors_num; i++) {
        var object_entries = Object.entries(json_objects[i][1]);

        for (let j = 0; j < object_entries.length; j++) {
            var element_value_id = json_objects[i][0].replace(/\s/g, '_') + '-' + object_entries[j][0].replace(/\s/g, '_') + '-value';
            var element_value = document.getElementById(element_value_id);

            if ((typeof element_value === 'undefined') || (element_value == null)) {
                break;
            }

            if (typeof object_entries[j][1] == 'number') {
                element_value.innerHTML = Number.parseFloat(object_entries[j][1]).toPrecision(6);
            } else if(typeof object_entries[j][1] == 'string'){
				element_value.innerHTML = object_entries[j][1];
			} else {
                element_value.innerHTML = 'NaN';
            }
        }
    }
}

function initDataLive() {
	fetch("/live")
		.then(response => {
		  if (response.ok) {
			/* The connection was successful*/
			return response.json();
		  } else {
			/* The connection was not successful*/
			throw new Error(`Error ${response.status}: ${response.statusText}`);
		  }
		})
		.then(data => {
			  /* Update the text label with the new data*/
			  console.log("Refresh Status OK");
			  createLiveTable(data);
		})
		.catch(error => {
		  /* Handle any errors that occurred*/
		  console.error(error);
		});
  }
  
  function getDataLive() {
	fetch("/live")
		.then(response => {
		  if (response.ok) {
			/* The connection was successful*/
			return response.json();
		  } else {
			/* The connection was not successful*/
			throw new Error(`Error ${response.status}: ${response.statusText}`);
		  }
		})
		.then(data => {
			  /* Update the text label with the new data*/
			  console.log("Refresh Status OK");
			  updateLiveTable(data);
		})
		.catch(error => {
		  /* Handle any errors that occurred*/
		  console.error(error);
		});
  }