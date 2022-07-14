const mongoose = require("mongoose");
const jwt = require("jsonwebtoken");
const bcrypt = require("bcrypt");
User = mongoose.model('User');

exports.register = function(req, res){
	try {
		const {first_name, last_name, email, emp_id, password} = req.body;
		//subl console.log(email, emp_id, password);

		if(!(email && password && first_name && last_name && emp_id)){
			res.status(400).send("All fields are required.");
		}

		const oldUser = await User.findOne({ email });

		if (oldUser) {
			return res.status(409).send("User already exists. Please login.");
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

		res.status(201).json(user);
	} catch (err) {
		console.log(err);
	}
}