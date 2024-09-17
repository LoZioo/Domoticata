
// VU meters refresh time.
const CONFIG_LOOP_PERIOD_MS = 1000;

// Maximum value that can be reached by VU meters (VA, W, I, V).
const CONFIG_VUMETER_MAX_VALUES = [3500, 3500, 14, 400];

// PowerMonitor data source URI.
const CONFIG_PM_DATA_URI = "/pm";

// [Array] VU meters canvases.
let vumeters_boxes;

// [Array] Labels above VU meters.
let vumeters_labels;

// App entry point.
$(async function(){
	await setup();
	setInterval(loop, CONFIG_LOOP_PERIOD_MS);
});

async function setup(){

	vumeters_boxes = [
		$("#p_va canvas").get(0),
		$("#p_w canvas").get(0),
		$("#i_rms canvas").get(0),
		$("#v_rms canvas").get(0)
	];

	vumeters_labels = [
		$("#p_va span"),
		$("#p_w span"),
		$("#i_rms span"),
		$("#v_rms span")
	];

	// VU meters spawn.
	for(let i=0; i<4; i++)
		vumeter(vumeters_boxes[i], {
			boxCountRed:		3,
			boxCountYellow:	5,
			boxCount:				17,

			boxGapFraction:	0.3,
			max:						CONFIG_VUMETER_MAX_VALUES[i]
		});
}

async function loop(){

	let data;
	try {
		data = await $.get(CONFIG_PM_DATA_URI);
	}

	catch(e){
		ajax_error(e);
		return;
	}

	set_vumeters_data(data, vumeters_boxes, vumeters_labels);
}

function set_vumeters_data(json_data, vumeters_boxes, vumeters_labels){
	for(let i=0; i<4; i++){
		let val;

		switch(i){
			case 0:
				val = parseInt(json_data.p.va);
				break;

			case 1:
				val = parseInt(json_data.p.w);
				break;

			case 2:
				val = parseFloat(json_data.i.rms).toFixed(2);
				break;

			case 3:
				val = parseFloat(json_data.v.rms).toFixed(1);
				break;
		}

		vumeters_boxes[i].setAttribute("data-val", val);
		vumeters_labels[i].html(val);
	}
}

function reset_vumeters_data(vumeters_boxes, vumeters_labels){
	for(let i=0; i<4; i++){
		vumeters_boxes[i].setAttribute("data-val", 0);
		vumeters_labels[i].html(0);
	}
}
