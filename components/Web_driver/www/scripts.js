var getDataIntervalID;
var getDataLiveIntervalID;
var current_tab = 1;

const preferencesData = {
	wifi_pass: "your_wifi_pass",
	main_alt: 200,
	drouge_alt: 0,
	rail_height: 2,
	max_tilt: 45,
	staging_delay: 0,
	staging_max_tilt: 45,
	auto_arming_time_s: 60,
	auto_arming: true,
	key: 12345678, /* Replace with your key value */
	lora_freq: 433125,
	lora_mode: true, /* true for network, false for exclusive */
	lora_tx_dbm: 0,
	lora_key: 0
};

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
	
	getPref();
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

let touchstartX = 0;
let touchendX = 0;
let touchstartY = 0;
let touchendY = 0;
    
function checkDirection() {
  /* Ignore if user wants to refresh page */
  /* Get the absolute vertical distance */
  let verticalDistance = Math.abs(touchstartY - touchendY);  
  
  /* Calculate the difference between start and end positions */
  let swipeDistance = touchendX - touchstartX;
  
  console.log("dX = " + swipeDistance + "   dY = " + verticalDistance);
  
  /* Check if the swipe distance meets your desired threshold */
  if ((Math.abs(swipeDistance) > 80) && (verticalDistance < 50)) {
  	/* Perform the action for the swipe gesture */
    if (swipeDistance > 0) {
      Tab_swipeRight(); /* Right swipe */
    } else {
      Tab_swipeLeft();  /* Left swipe */
    }
  }
}

document.addEventListener('touchstart', e => {
  touchstartX = e.changedTouches[0].screenX;
  touchstartY = e.changedTouches[0].screenY;
});

document.addEventListener('touchend', e => {
  touchendX = e.changedTouches[0].screenX;
  touchendY = e.changedTouches[0].screenY;
  checkDirection();
});

function Tab_swipeRight() {
	switch(current_tab){
		case 1:
		break;
		
		case 2:
		SelectSection_Home();
		break;
		
		case 3:
		SelectSection_Storage();
		break;
		
		case 4:
		SelectSection_Ign();
		break;
		
		case 5:
		SelectSection_Settings();
		break;
	}
}

function Tab_swipeLeft() {
	switch(current_tab){
		case 1:
		SelectSection_Storage();
		break;
		
		case 2:
		SelectSection_Ign();
		break;
		
		case 3:
		SelectSection_Settings();
		break;
		
		case 4:
		SelectSection_Live();
		break;
		
		case 5:
		
		break;
	}
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
	
	current_tab = num;
	
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

/*-------------------- Main tab -------------------------------*/
function main_arming_handler () {
	console.log("Main - Arming button pressed");
	if(confirm('Do you want to Arm?\n\nAfter that command Flight Computer will be ready to flight and igniters will be armed!\n\nUse with caution!')) { 
		/*POST_simple("/delete/storage/meas.bin", '');*/
	}
}

function main_disarming_handler () {
	console.log("Main - Arming button pressed");
	if(confirm('Do you want to Disarm?\n\nAfter that command Flight Computer will be disarmed and will not work during flight!\n\nIt will be safe to approach :)')) { 
		/*POST_simple("/delete/storage/meas.bin", '');*/
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

function vibrate(time){
	/* Check if the Vibration API is supported */
	if ('vibrate' in navigator) {
	  /* Vibrate for 200ms */
	  navigator.vibrate(time);
	}
}

/*--------------------- Igniters tab ------------------------------*/
function ign_ign1_unlock_handler () {
	if(document.getElementById('button-ign1-fire').disabled == false){
		/*document.getElementById('button-ign1-unlock').disabled = true;*/
		document.getElementById('button-ign1-unlock').textContent = "UNLOCK"   
		
		/* Enable the fire button */
		document.getElementById('button-ign1-fire').disabled = true;  
		
        /* Update the lock status label */ 
		var lockStatusLabel = document.getElementById('label-igniter1-lock-status');             
		lockStatusLabel.textContent = "Status Locked";
		lockStatusLabel.style.color = "black";
		return;
	}
	
	vibrate(200); 
	
	/* Set a timeout to execute the code after 200ms */
	setTimeout(function (){
			if(confirm('Are you sure? Igniter 1 will be unlocked!')) {
				/* Rename the unlock button */
				document.getElementById('button-ign1-unlock').textContent = "SECURE"   
				
				/* Enable the fire button */
				document.getElementById('button-ign1-fire').disabled = false;  
				
		        /* Update the lock status label */ 
				var lockStatusLabel = document.getElementById('label-igniter1-lock-status');             
				lockStatusLabel.textContent = "Unlocked!";
				lockStatusLabel.style.color = "red";
			}
		}, 200);
	
}

function ign_ign2_unlock_handler () {
	if(document.getElementById('button-ign2-fire').disabled == false){
		/*document.getElementById('button-ign2-unlock').disabled = true;*/
		document.getElementById('button-ign2-unlock').textContent = "UNLOCK"   
		
		/* Enable the fire button */
		document.getElementById('button-ign2-fire').disabled = true;  
		
        /* Update the lock status label */ 
		var lockStatusLabel = document.getElementById('label-igniter2-lock-status');             
		lockStatusLabel.textContent = "Status Locked";
		lockStatusLabel.style.color = "black";
		return;
	}
	
	vibrate(200);
	
	/* Set a timeout to execute the code after 200ms */
	setTimeout(function() {
			if(confirm('Are you sure? Igniter 2 will be unlocked!')) {
				/* Rename the unlock button */
				document.getElementById('button-ign2-unlock').textContent = "SECURE"   
				
				/* Enable the fire button */
				document.getElementById('button-ign2-fire').disabled = false;  
				
				/* Update the lock status label */
				var lockStatusLabel = document.getElementById('label-igniter2-lock-status');             
				lockStatusLabel.textContent = "Unlocked!";
				lockStatusLabel.style.color = "red";
			}
		}, 200);
}

function ign_ign3_unlock_handler ()  {
	if(document.getElementById('button-ign3-fire').disabled == false){
		/*document.getElementById('button-ign3-unlock').disabled = true;*/
		document.getElementById('button-ign3-unlock').textContent = "UNLOCK"   
		
		/* Enable the fire button */
		document.getElementById('button-ign3-fire').disabled = true;  
		
        /* Update the lock status label */ 
		var lockStatusLabel = document.getElementById('label-igniter3-lock-status');             
		lockStatusLabel.textContent = "Status Locked";
		lockStatusLabel.style.color = "black";
		return;
	}
	vibrate(200); 
	
	/* Set a timeout to execute the code after 200ms */
	setTimeout(function() {
			if(confirm('Are you sure? Igniter 3 will be unlocked!')) {
				/* Rename the unlock button */
				document.getElementById('button-ign3-unlock').textContent = "SECURE" 
				
				/* Enable the fire button */ 
				document.getElementById('button-ign3-fire').disabled = false;   
				
				/* Update the lock status label */              
				var lockStatusLabel = document.getElementById('label-igniter3-lock-status');
				lockStatusLabel.textContent = "Unlocked!";
				lockStatusLabel.style.color = "red";
			}
		}, 200);
}

function ign_ign4_unlock_handler ()  {
	if(document.getElementById('button-ign4-fire').disabled == false){
		/*document.getElementById('button-ign4-unlock').disabled = true;*/
		document.getElementById('button-ign4-unlock').textContent = "UNLOCK"   
		
		/* Enable the fire button */
		document.getElementById('button-ign4-fire').disabled = true;  
		
        /* Update the lock status label */ 
		var lockStatusLabel = document.getElementById('label-igniter4-lock-status');             
		lockStatusLabel.textContent = "Status Locked";
		lockStatusLabel.style.color = "black";
		return;
	}
	vibrate(200); 
	
	/* Set a timeout to execute the code after 200ms */
	setTimeout(function() {
		if (confirm('Are you sure? Igniter 4 will be unlocked!')) {
			/* Rename the unlock button */
				document.getElementById('button-ign4-unlock').textContent = "SECURE" 

			/* Enable the fire button */
			document.getElementById('button-ign4-fire').disabled = false;

			/* Update the lock status label */
			var lockStatusLabel = document.getElementById('label-igniter4-lock-status');
			lockStatusLabel.textContent = 'Unlocked!';
			lockStatusLabel.style.color = 'red';
		}
	  }, 200);
}

function ign_ign1_fire_handler() {
	console.log("Igniters - Fire ign 1");
	vibrate(200); 
	POST_simple("/cmd", '{"cmd":"ign_set","arg1":1,"key":2137}');
}

function ign_ign2_fire_handler() {
	console.log("Igniters - Fire ign 2");
	vibrate(200); 
	POST_simple("/cmd", '{"cmd":"ign_set","arg1":2,"key":2137}');
}

function ign_ign3_fire_handler() {
	console.log("Igniters - Fire ign 3");
	vibrate(200); 
	POST_simple("/cmd", '{"cmd":"ign_set","arg1":3,"key":2137}');
}

function ign_ign4_fire_handler() {
	console.log("Igniters - Fire ign 4");
	vibrate(200); 
	console.log(POST_simple("/cmd", '{"cmd":"ign_set","arg1":4,"key":2137}'));
}

function POST_simple(url, data) {
  return fetch(url, {method: "POST", headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: data});
}

function SysMgr_statusToLabel(status, label_id){
	if(status == 0x01){
		document.getElementById(label_id).textContent = "OK";
		document.getElementById(label_id).style.color = "green";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
		
	if(status == 0x02){
		document.getElementById(label_id).textContent = "WARNING";
		document.getElementById(label_id).style.color = "red";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
	if(status == 0x04){
		document.getElementById(label_id).textContent = "FAIL";
		document.getElementById(label_id).style.color = "red";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
	
	document.getElementById(label_id).textContent = "ERROR";
	document.getElementById(label_id).style.color = "red";
	document.getElementById(label_id).style.fontWeight = "bold"
	return;
}

function SysMgr_armingStatusToLabel(status, label_id){
	if(status == 0x01){
		document.getElementById(label_id).textContent = "ARMED!";
		document.getElementById(label_id).style.color = "green";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
		
	if(status == 0x02){
		document.getElementById(label_id).textContent = "Disarmed!";
		document.getElementById(label_id).style.color = "red";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
	if(status == 0x04){
		document.getElementById(label_id).textContent = "ERROR :(";
		document.getElementById(label_id).style.color = "red";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
	
	document.getElementById(label_id).textContent = "ERROR";
	document.getElementById(label_id).style.color = "red";
	document.getElementById(label_id).style.fontWeight = "bold"
	return;
}

function GpsFix_fixToLabel(status, label_id){
	if(status == 0){
		document.getElementById(label_id).textContent = "No Fix";
		document.getElementById(label_id).style.color = "red";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
	if(status == 1){
		document.getElementById(label_id).textContent = "Fix OK";
		document.getElementById(label_id).style.color = "green";
		document.getElementById(label_id).style.fontWeight = "bold"
		return;
	}
		
	document.getElementById(label_id).textContent = "ERROR";
	document.getElementById(label_id).style.color = "red";
	document.getElementById(label_id).style.fontWeight = "bold";
	return;
}

/* Function to retrieve data from the remote server*/
function getDataStatus() {
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
			document.getElementById("label-status-serial-number").textContent 		= data.configuration.serial_number;
			document.getElementById("label-status-software-version").textContent 	= data.configuration.software_version;
			
			SysMgr_armingStatusToLabel(data.sysMgr.sysmgr_arm_state, "label-status-arming");
			document.getElementById("label-status-timestamp_ms").textContent 	= data.system.timestamp_ms + " ms";
			
			SysMgr_statusToLabel(data.sysMgr.sysmgr_system_status, "label-status-system");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_analog_status, "label-status-analog");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_lora_status, "label-status-lora");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_adcs_status, "label-status-adcs");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_storage_status, "label-status-storage");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_sysmgr_status, "label-status-sysmgr");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_utils_status, "label-status-utils");
			SysMgr_statusToLabel(data.sysMgr.sysmgr_web_status, "label-status-web");
			
			document.getElementById("label-status-pressure").textContent 	= data.sensors.pressure;
			document.getElementById("label-status-angle").textContent 		= data.sensors.rocket_tilt.toFixed(2) + " deg";
			GpsFix_fixToLabel(data.sensors.gpsfix, "label-status-gpsstat");
			document.getElementById("label-status-gpssats").textContent 		= data.sensors.gpssats;
			
			if(data.system.battery_voltage > 3.0)
				document.getElementById("label-status-vbat").textContent 		= data.system.battery_voltage.toFixed(2) + " V";
			else 
				document.getElementById("label-status-vbat").textContent 		= "LOW!";
			
			IGN_contToLabel(data.igniters[0].continuity, "label-igniter1-cont-status");
			IGN_contToLabel(data.igniters[1].continuity, "label-igniter2-cont-status");
			IGN_contToLabel(data.igniters[2].continuity, "label-igniter3-cont-status");
			IGN_contToLabel(data.igniters[3].continuity, "label-igniter4-cont-status");
	  })
	  .catch(error => {
		/* Handle any errors that occurred*/
		console.error(error);
	  });
}

function IGN_contToLabel(cont, label){
	let color, text;
	
	if(cont == 0){
		text 	= "Missing!";
		color 	= "red";
	} else if(cont == 1){
		text 	= "Connected";
		color 	= "green";
	} else {
		text 	= "BAT missing!";
		color 	= "orange";
	}
	
	document.getElementById(label).textContent = text;
	document.getElementById(label).style.color = color;
	document.getElementById(label).style.fontWeight = "bold"
}

function webInit(){
	TabsInit();
	/* Set an interval to retrieve new data every 10 second*/
	setInterval(getDataStatus, 1000);
	initDataLive();
	getDataStatus();
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
  
  


/* Function to calculate CRC16 checksum */
function calculateCRC16(data) {
	 const poly = 0x1021;
    let crc = 0xffff;

    for (let i = 0; i < data.length; i++) {
        crc ^= (data.charCodeAt(i) << 8);

        for (let j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? ((crc << 1) ^ poly) : (crc << 1);
        }
    }

    return crc & 0xffff;
}

/* Function to send the JSON data as a POST request */
function sendPreferencesData(preferencesData) {
	if(confirm('Save and restart? If you changed WiFi password then change your phone settings too.')) { 
		/* Calculate CRC16 checksum for the JSON data */
		const jsonData = JSON.stringify(preferencesData,null,"	");
		const crc16 = calculateCRC16(jsonData);
		
		console.log("Sending preferencesData:", preferencesData);
		
		/* Define the URL to which you want to send the POST request*/
		const apiUrl = '/config'; /* Replace with your API URL */
	
		/* Send the JSON data as a POST request */
		console.log(POST_simple("/config", crc16 + ',' + jsonData));
		
		setTimeout(getPref(), 5000);
	}
}

function formatWifiPass(wifiPass) {
	if (wifiPass.length > 12) {
		return wifiPass.substring(0, 12);
	} else {
		return wifiPass;
	}
}


function updatePreferencesData() {
	preferencesData.rail_height 		= parseInt(1000 * document.getElementById("pref-launchpad-height").value);
	preferencesData.main_alt 			= parseInt(document.getElementById("pref-main-alt").value);
	preferencesData.drouge_alt 			= parseInt(document.getElementById("pref-drouge-alt").value);
	preferencesData.staging_delay 		= parseInt(1000 * document.getElementById("pref-staging-delay").value);
	preferencesData.staging_max_tilt 	= parseInt(document.getElementById("pref-staging-tilt").value);
	preferencesData.auto_arming_time_s 	= parseInt(document.getElementById("pref-autoarm-delay").value);
	preferencesData.auto_arming 		= parseInt(document.getElementById("pref-autoarm-enable").selectedIndex);
	preferencesData.lora_freq 			= parseInt(1000 * document.getElementById("pref-lora-frequency").value);
	preferencesData.lora_mode			= parseInt(document.getElementById("pref-lora-mode").selectedIndex);
	preferencesData.lora_key			= parseInt(document.getElementById("pref-lora-key").value);
	preferencesData.lora_tx_dbm			= 0;
	
	preferencesData.wifi_pass = document.getElementById("pref-wifi-pass").value;
	

	/* Add similar lines for other properties as needed */

	console.log("Updated preferencesData:", preferencesData);
}

function preferencesDataLoraMode(state) {
	preferencesData.lora_mode = state;
}

function preferencesDataAutoarming(state) {
	preferencesData.auto_arming = state;
}

function getPref() {
	fetch("/pref")
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
				console.log("Refresh Pref OK");
				console.log(data);
				document.getElementById("pref-launchpad-height").value 	= parseFloat(data.pref_launchpad_height * 0.001);
				document.getElementById("pref-main-alt").value 			= parseInt(data.pref_main_alt);
				document.getElementById("pref-drouge-alt").value 		= parseInt(data.pref_drouge_alt);
				document.getElementById("pref-staging-delay").value 	= parseFloat(data.pref_staging_delay * 0.001);
				document.getElementById("pref-staging-tilt").value 		= parseInt(data.pref_staging_tilt);
				document.getElementById("pref-autoarm-delay").value 	= parseInt(data.pref_autoarm_delay);
				document.getElementById("pref-lora-frequency").value 	= parseFloat(data.pref_lora_frequency * 0.001);
				document.getElementById("pref-wifi-pass").value 		= data.pref_wifi_pass
				document.getElementById("pref-autoarm-enable").selectedIndex = parseInt(data.pref_autoarming);
				document.getElementById("pref-lora-mode").selectedIndex = parseInt(data.pref_lora_mode);
				document.getElementById("pref-lora-key").value			= parseInt(data.pref_lora_key);
				updatePreferencesData();
	
	preferencesData.wifi_pass = document.getElementById("pref-wifi-pass").value;
		})
		.catch(error => {
		  /* Handle any errors that occurred*/
		  console.error(error);
		});
}
