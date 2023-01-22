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
			document.getElementById("label-status-web").textContent 	= data.sysMgr.Web_driver;
			document.getElementById("label-status-pressure").textContent 	= data.sensors.pressure;
			document.getElementById("label-status-altitude").textContent 	= data.sensors.altitude;
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
}

