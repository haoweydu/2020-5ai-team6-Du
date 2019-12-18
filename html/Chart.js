// Load the Visualization API and the corechart package.
google.charts.load('current', { 'packages': ['corechart'] });

// Set a callback to run when the Google Visualization API is loaded.
google.charts.setOnLoadCallback(drawChart);

// Callback that creates and populates a data table,
// instantiates the pie chart, passes in the data and
// draws it.
function drawChart() {

    // Create the data table.
    var data = google.visualization.arrayToDataTable([
        ['Task', 'Hours per Day'],
        ['periodo silenzioso',     11],
        ['periodo di media',      5],
        ['periodo rumoroso',      2]
    ]);
    var data1 = google.visualization.arrayToDataTable([
        ['Year', 'aula1', 'aula2'],
        ['2013', 1000, 400],
        ['2014', 1170, 460],
        ['2015', 660, 1120],
        ['2016', 1030, 540]
    ]);
    var options1 = {
        title: 'livelli rumore',
        hAxis: { title: 'Year', titleTextStyle: { color: '#333' } },
        vAxis: { minValue: 0 }
    };
    var chart1 = new google.visualization.AreaChart(document.getElementById('chart_div1'));
    chart1.draw(data1, options1);

    // Set chart options
    var options = {
        title: 'Confronto livelli rumore',
        slices: [{color: "#006400"},{color: '#FFD300'},{color: 'red'}]
    };

    // Instantiate and draw our chart, passing in some options.
    var chart = new google.visualization.PieChart(document.getElementById('chart_div'));
    chart.draw(data, options);
    

}