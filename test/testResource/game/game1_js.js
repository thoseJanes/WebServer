// JavaScript Document
var createc = document.querySelector(".createcharacter");
var ptable=[["力量","魔法","防御","魔抗","生命","魔力","武器","防具","剑术","弓术","运气","意志"],["学习","育成","狩猎","驯服","精准","回避","精力","回复","修理","酿造","锻造","附魔"],["机关","陷阱","话术","诈术","统帅","分析","观察","魅惑","威慑","直觉","亲和","伪装"]];
var batable=[["农夫之子","流浪者","野人"],["沉睡中醒来","受到诅咒的弱龙"]]
var crace=document.getElementsByName("race")[0];
var lh_point = 20;
var lbpDis = $("lhpoint");

crace.addEventListener('change',function(){
	$("cbackground").innerHTML="";
	switch (this.value){
		case "human":
			addinput($("cbackground"),batable[0],"radio","background");
			break;
		case "dragon":
			addinput($("cbackground"),batable[1],"radio","background");
			break;
	}
});

function $(Id){
	return document.getElementById(Id);
}

function gameBegin(ob){

	ob.style.transition = "all 0.6s";
	ob.style.opacity=0;
	var potentialtable = document.getElementById("potentialTable");
	potential(potentialtable);
	createc.style.display="block";
	createc.style.opacity=0;
	setTimeout(function(){ob.parentNode.removeChild(ob);},590);
	setTimeout(gamestart,600);
}

function gamestart(){
	createc.style.transition = "all 0.6s";
	createc.style.opacity=0.6;
	addinput($("cbackground"),batable[0],"radio","background");
	
	var potential_block = document.getElementsByName("potential");
	lbpDis.innerHTML='灵魂：'+lh_point.toString();
	//添加事件
	for(var i=0,len=potential_block.length;i<len;i++){
	var u=potential_block[i];
	u.addEventListener('change',function(){
		if (this.checked == 1){
			if(lh_point>=5){
				lh_point=lh_point-5;
			}
			else{
				window.alert("灵魂值不足！");
				this.checked=0;}
		}
		else{
			lh_point=lh_point+5;
		}
		lbpDis.innerHTML='值：'+lh_point.toString();
	});
}
}

//根据种族进行身份选项的改变
function addinput(ob,table,type,name){
	var tr;
	var pot;
	var t;
	var div;
	for (var i=0,len=table.length;i<len;i++){
		tr=document.createElement("br");
		pot=document.createElement("input");
		pot.type=type;
		pot.name=name;
		pot.value=table[i];
		t=document.createTextNode(table[i]);
		div=document.createElement("div");
		div.appendChild(pot);
		div.appendChild(t);
		ob.appendChild(div);
		ob.appendChild(tr);
	}
}

//在某一元素下添加一些多选控件
function potential(ob){
	var tr = [];
	var pot = [];
	for (var i=0,len2=ptable.length;i<len2;i++){
		tr.push([]);
		pot.push([]);
		tr[i].push(document.createElement("tr"));
		for (var j=0,len=ptable[i].length;j<len;j++){
			tr[i].push(document.createElement("td"));
			pot[i].push(document.createElement("input"));
			pot[i][j].type="checkbox";
			pot[i][j].name="potential";
			pot[i][j].value=ptable[i][j];
			pot[i][j].className="checkbox";
			tr[i][j+1].innerHTML=ptable[i][j];
			tr[i][j+1].appendChild(pot[i][j]);
			tr[i][0].appendChild(tr[i][j+1]);
		}
		ob.appendChild(tr[i][0]);
	}
	pot = null;
	tr = null;
	ob.appendChild(document.createElement("br"));
	tr = null;
}

//获取表单的值
function formget(ob){
	var select;
	var form;
	var formcontent=[];
	//获取input元素的值
	form = ob.getElementsByTagName("input");
	for(var i=0,len=form.length;i<len;i++){
		if(form[i].type=="checkbox"){
			if(form[i].checked==true){
			//潜能类二元值【potential，某潜能】
			formcontent.push(["potential",form[i].value])
			}}
		else if(form[i].type=="radio"){
			if(form[i].checked==true){
				//背景二元值【background，某背景】
			formcontent.push(["background",form[i].value])
			}}
		else{
			formcontent.push([]);
			//基本信息类二元值【某基本信息，某值或类型】
			formcontent[i].push(form[i].name);
			formcontent[i].push(form[i].value);
		}
	}
	//获取select类元素的值
	form = ob.getElementsByTagName("select");
	for(i=0,len=form.length;i<len;i++){
		select = form[i].getElementsByTagName("option");
		for(var j=0,len2=select.length;j<len2;j++){
			if(select[j].selected==true){
				//基本信息类二元值【某基本信息，某值或类型】
					formcontent.push([form[i].name,select[j].value]);
			}
		}
	}
	alert(formcontent)
}

