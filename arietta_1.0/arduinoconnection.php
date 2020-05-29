<?php

$servername = "localhost";
$username = "root";
$password = "";
$dbname = "arietta";


$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
  die("Connection failed: " . $conn->connect_error);
}




$temperatura = $_POST["temp"];  //ricevo i dati inviati da Arduino
$umidita = $_POST["umi"];
$suono = $_POST["suono"];




$sql = "INSERT INTO `data` (`temperatura`, `umidita`, `suono`)   //inserisco i dati all'interno del database arietta, nello specifico nella table nominata data
		VALUES ('".$temperatura."','".$umidita."', '".$suono."')"; 


if ($conn->query($sql) === TRUE) {
  echo "Nuovo record creato con successo";
} else {
  echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>