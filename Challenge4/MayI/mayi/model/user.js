const mongoose = require("mongoose");

const userSchema = new mongoose.Schema({
	first_name: {type: String, default: null},
	last_name: {type: String, default: null},
	email: {type: String, unique: true},
	emp_id: {type:Number, unique: true},
	password: {type: String},
	token: {type: String},
	ip: {type: String},
	reason: {type: String}
});
userSchema.methods.comparePassword = function(password){
	return bcrypt.compareSync(password, this.password)
};

module.exports = mongoose.model("user",userSchema);