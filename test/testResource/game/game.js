// JavaScript Document
var canvas = document.getElementById("game");

var ctx = canvas.getContext && canvas.getContext("2d");
if(!ctx) {
	alert('please upgrade your bowser');
}else{
	startGame();
}
function startGame(){
	//Let's get to work
/***	ctx.fillStyle = "#FFFF00";
	ctx.fillRect(50,100,380,400);
	ctx.fillStyle = "rgba(0,0,128,0.8);";
	ctx.fillRect(25,50,380,400);
	***/
	var img = new image();
	img.onload = function(){
		ctx.drawimage(img,100,100,200,200);
	}
	img.src = "sprites.png";
}