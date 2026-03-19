<?php
	$conn = mysqli_connect("localhost", "iot", "pwiot");
	mysqli_select_db($conn, "smart_trashcan");
	$query = "select name, date, time, trash_height_1, trash_height_2 from sensor order by date desc, time desc limit 20";
	$result = mysqli_query($conn, $query);

	$data = array(array('Time','height 1','height 2'));

	if($result)
	{
		while($row = mysqli_fetch_array($result))
		{
			array_push($data, array($row['date']."\n".$row['time'], intval($row['trash_height_1']),intval($row['trash_height_2'])));
		}
	}

	$options = array(
			'title' => 'Real-time Height Measurement Data',
			'width' => 1000, 'height' => 400,
			'curveType' => 'function',
			'vAxis' => array(
				'title' => 'height (cm)',
				'viewWindow' => array(
					'min' => 0,
					'max' => 30),
			)
		);

?>

<script src="//www.google.com/jsapi"></script>
<script>
var data = <?=json_encode($data) ?>;
var options = <?= json_encode($options) ?>;

google.load('visualization', '1.0', {'packages':['corechart']});

google.setOnLoadCallback(function() {
	var chart = new google.visualization.LineChart(document.querySelector('#chart_div'));
	chart.draw(google.visualization.arrayToDataTable(data), options);
	});
	</script>
<div id="chart_div"></div>
