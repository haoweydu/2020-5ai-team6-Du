<?php
//Header to JSON
header("Content-Type: application/json");

$user = 'root';
$password = '';
$db = 'arietta';
$host = 'localhost';

//Connecting
$mysqli = new PDO("mysql:host=$host; dbname=$db", $user, $password);

$stmt = $mysqli->prepare("SELECT temperatura AS y, date AS x FROM data ORDER BY date"); //prelevo la temperatura e la data come x e y dalla table arietta.data
$stmt->execute(); // esegue query
$arr = $stmt->fetchAll(PDO::FETCH_ASSOC); // aggiunge le row ad un array associativo
echo json_encode($arr);
?>
