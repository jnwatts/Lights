function do_action(action, args) {
	return $.post(window.location, {action: action, args: args}, () => {}, "json");
}

function simple_action(e) {
	var button = $(e.target)
	var action = button.data('action');
	var args = button.data('args');

	if (!action) {
		console.log(button);
		return;
	}

	if (args) {
		args = JSON.stringify(args);
	}
	do_action(action, args).done((response) => {
		if (response.msg) {
			$('#msg').html(response.msg);
		} else {
			$('#msg').html(JSON.stringify(response, null, 2));
		}
	});
}

function select_preset(e) {
	e.target = $(e.target).parent()[0];
	simple_action(e);
}

function preset_color(p) {
    p.shift()
    var p_c = p.map( (c, i) => {
        return chroma(window.colors[i]).set('hsl.l', '*'+c);
    });

    var blend_type = 'lighten';
    var f = chroma('#000');
    p_c.forEach( (c) => { f = chroma.blend(f, c, blend_type); } );
    return f;
}

function fill_presets() {
	var presets = $('#presets');

	for (p in backend.presets) {
		var button = $('<ons-list-item tapable ripple data-action="target" data-args="'+JSON.stringify(backend.presets[p])+'" />')
			.text(p)
			.on('click', select_preset);
        var color = $('<div class="color-box" style="margin-left: 8px; width: 10px; height: 10px">&nbsp;</div>').css('background-color', preset_color(backend.presets[p]));
        button.append(color);
		presets.append(button);
	}
}

function ready() {
	if (backend.msg) {
		$('#msg').html(backend.msg);
	}

	$('.action').on('click', simple_action);

    cw = '#FFFFFF'; // Australian N14 "White"
    ww = '#F2E8D6'; // Australian X33 "Warm white"
    r = '#F20000';
    g = '#00F200';
    b = '#0000F2';
    window.colors = [cw, ww, r, g, b];

	fill_presets();

	$('#msg').html(JSON.stringify(backend, null, 2));

}
