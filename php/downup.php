<?php

define ( "MYSQL_HOST_M", "localhost" );
define ( "MYSQL_PORT", "80" );
define ( "MYSQL_USER", "root" );
define ( "MYSQL_PASS", "vilink" );
define ( "MYSQL_DB_NAME", "app_vilink" );

if ($_GET['data'] && ($_GET['token'] == "doubleq")) {//可以改token,这相当于密码，在Arduino端改成相应的值即可
	$con = mysql_connect(MYSQL_HOST_M.':'.MYSQL_PORT, MYSQL_USER, MYSQL_PASS); 
	$data = $_GET['data'];
	mysql_select_db(MYSQL_DB_NAME, $con);//要改成相应的数据库名

	$result = mysql_query("SELECT * FROM ledswitch"); 
	while($arr = mysql_fetch_array($result)){//找到需要的数据的记录，并读出状态值
		if ($arr['ID'] == 1) {
			$state = $arr['state'];
		}
	}
	$dati = date("h:i:sa");//获取时间
	$sql ="UPDATE sensor SET timestamp='$dati',data = '$data'
	WHERE ID = '1'";//更新相应的传感器的值
	if(!mysql_query($sql,$con)){
		die('Error: ' . mysql_error());//如果出错，显示错误
	}
	mysql_close($con);
	echo "ledswitch={".$state."}";//返回状态值，加“{”是为了帮助Arduino确定数据的位置
}else{
    echo "Permission Denied";//请求中没有type或data或token或token错误时，显示Permission Denied
}
?>
