if (process.env.NODE_ENV !== 'production') {
  require('dotenv').config();
}
require("./config/database").connect();
const express = require("express");
const crypt3 = require('crypt3-passwd')
const User = require("./model/user");
const Request = require("./model/request");
const bcrypt = require("bcrypt");
const {exec} = require("child_process");
var path = require('path');
const app = express();
const cookieParser = require("cookie-parser");
const jwt = require("jsonwebtoken");
const auth = require("./middleware/auth");
app.use(cookieParser());

app.set('view engine', 'ejs');
const saltRounds = 10;
app.use(express.urlencoded());

app.use(express.static(__dirname + '/public'));

/*
const newDB = async () => {
	try {
		User.deleteMany({});
		Request.deleteMany({});
		console.log("Emptied MongoDB Collections!");
	} catch(err) {
		console.log(err);
	}
}
*/

app.get("/login", (req, res) =>{
	res.sendFile(path.join(__dirname, '/public', "login.html"));
});

app.post("/login", async (req, res) => {
	try{
		const {email, password} = req.body;
		const user = await User.findOne({ email });

		if (!user || !bcrypt.compareSync(password, user.password)){
			return res.status(418).send("Error: User doesn't exist or password is incorrect");
		}
		const emp_id = user.emp_id;
		const token = jwt.sign(
			{user_id: user._id, email, emp_id},
			process.env.TOKEN_KEY,
			{
				expiresIn: "6h",
			}
		);
		user.token = token;
		return res
			.cookie("access_token", token, {
				httpOnly: true,
				secure: process.env.NODE_ENV === "production",
			})
			.status(200)
			.redirect("/status");
		//return res.status(201).json(user);
	} catch (err) {
		console.log(err);
	}
});
app.get("/register", (req,res) => {
	res.sendFile(path.join(__dirname, '/public', "register.html"));
});
app.post("/register", async (req, res) =>{
	try {
		const {first_name, last_name, email, emp_id, password} = req.body;
		//console.log(email, emp_id, password);

		if(!(email && password && first_name && last_name && emp_id)){
			return res.status(400).send("All fields are required.");
		}

		const oldUser_emp_id = await User.findOne({ emp_id });
		const oldUser_email = await User.findOne({ email });

		if (oldUser_emp_id || oldUser_email) {
			return res.status(409).send("User already exists (same email or employee ID). Please login.");
		}

		encPassword = await bcrypt.hash(password, saltRounds);

		const user = await User.create({
			first_name,
			last_name,
			email: email.toLowerCase(),
			emp_id,
			password: encPassword
		});

		const token = jwt.sign(
			{user_id: user._id, email, emp_id},
			process.env.TOKEN_KEY,
			{
				expiresIn: "6h",
			}
		);
		user.token = token;
		return res.redirect("/login");
		//return res.status(201).json(user);
	} catch (err) {
		console.log(err);
	}
});

app.get("/admin",auth.verifyAdmin, async(req, res) => {
	//res.status(200).send("Welcome to admin!");
	var results = [];
	var count = 0;
	var requests = await Request.find().exec(function (err, result){
			if (err){
				console.log(err);
				return res.result(500).send("Apologies, something went wrong internally.");
			}
			//console.log(result);
			//return res.status(200).send(result);
			//console.log(result);
			//return res.status(200).send("status");
			results.push(result);
			res.render('admin.ejs',{requests: results});
		});
	/*
	var stream = await Request.find().stream();

	stream.on('data', function (doc) {
		results.push(doc);
		count++;
	})
	stream.on('error', function(err){
		console.log(err);
	})
	stream.on('close', function(){
		res.send(count);
	})
	//console.log(cursor);
	//res.send(cursor);
		
	
	//cursor.forEach(function(item) {
		//results.push(item);
		//count++;
	//});
		/*
			if(item == null){
				res.status(200).send({"Number of requests": count});
				//render status.html with the items
			}
			else if(item.status == "awaiting approval"){
				count = count + 1;
			}
			*/
		//});
		
});
app.post("/approve", auth.verifyAdmin, async(req, res) =>{
	function makeRand(option){
		var result = '';
		var uname_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		var pass_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()?,.-_+{}";
		var length = pass_characters.length;
		if(option == 1){
			for (var i = 0; i < 8; i++){
				result += uname_characters.charAt(Math.floor(Math.random() * length));
			}
			return result;
		}
		else{
			for (var i = 0; i < 8; i++){
				result += pass_characters.charAt(Math.floor(Math.random() * length));
			}
			return result;
		}
	}
	try{
		const temp_uname = makeRand(1);
		const temp_pass = makeRand(2);
		var creds = temp_uname;
		creds +=":";
		creds+=temp_pass;
		const hashed_pass = crypt3.encrypt(temp_pass);
		console.log("Making the following user: "+ creds + " hash: " + hashed_pass);
		const {email, ip} = req.body;
		var command = "ssh -i /keys/AgentKey MayIAgent@";
		command += ip;
		//command += " 'useradd -G sudo,MayIAccounts -p \"";
		command += " '/MayI/createUser.sh ";
		//command += hashed_pass;
		command += temp_uname;
		//command += "\" ";
		command += " "
		command += temp_pass;
		//command += temp_uname;
		command += "'"
		creds = "Approved! Credentials are " + creds;
		console.log("command to run: "+ command);
		exec(command, (error, stdout, stderr) =>{
			if(error){
				console.log(error);
			}
			if (stderr){
				console.log(stderr);
			}
			console.log('stdout: ${stdout}');
		});
		const update = {status: creds}
		//const request_to_approve = await Request.findOne({ email:email, ip:ip });
		let doc = await Request.findOneAndUpdate({email:email, ip:ip}, update, {new: true});
		return res.render('approved.ejs', {ip: doc.ip, email:doc.email})
	} catch (err){
		console.log(err);
		return res.status(500).send("Apologies, something went terribly wrong...");
	}
})
app.get("/status",auth.verifyToken, async (req, res) => {
	try{
		const token =
			req.body.token || req.query.token || req.headers["x-access-token"] || req.cookies.access_token;
		const decoded_token = jwt.verify(token, process.env.TOKEN_KEY);
		const decoded_email = decoded_token.email;
		/*
		var cursor = Request.find(({ decoded_email }));
		const count = 0;
		cursor.each(function(err, item) {
			if(item == null){
				//render status.html with the items
			}
		})
		*/
		//console.log(decoded_email);
		var requests = await Request.find({ email:decoded_email }).exec(function (err, result){
			if (err){
				console.log(err);
				return res.result(500).send("Apologies, something went wrong internally.");
			}
			//console.log(result);
			res.render('status.ejs',{requests: result})
			//return res.status(200).send(result);
		});
		//var requests = Request.find({decoded_email}), function (err, )
		//return res.status(200).sned(requests);
		/* requests.each(function(err, item) {
			if (item == null) {
				return res.status(200).send("No requests");
			}

		})
		*/
	} catch (err) {
		console.log(err);
	}
})

app.get("/",(req,res) => {
	res.sendFile(path.join(__dirname, '/public', "index.html"));;
});

app.get("/request", auth.verifyToken,(req,res) => {
	res.sendFile(path.join(__dirname, '/public', "request.html"));
});

app.post("/request", auth.verifyToken, async (req,res) => {
	try{
		const {ip, reason} = req.body;
		if(!(ip && reason)){
			return res.status(400).send("All fields are required.");
		}
		const token =
		req.body.token || req.query.token || req.headers["x-access-token"] || req.cookies.access_token;
		const decoded_token = jwt.verify(token, process.env.TOKEN_KEY);
		const decoded_email = decoded_token.email;
		const pending = await Request.findOne({ ip });
		if (pending) {
			return res.status(409).send("The requested IP already is already pending approval. Please wait your turn.");
		}
		const request = await Request.create({
			ip,
			reason,
			email: decoded_email,
			status: "awaiting approval"
		});
		return res.redirect("/status");
	} catch (err) {
		console.log(err);
	}
});

app.get("/logout", auth.verifyToken, (req, res) => {
	return res
		.clearCookie("access_token")
		.status(200)
		.sendFile(path.join(__dirname, '/public', "logout.html"));
});


module.exports = app;

