const mongoose = require("mongoose");

const requestSchema = new mongoose.Schema({
	ip: {type: String},
	reason: {type: String},
	email:  {type: String},
	status: {type: String}
})

module.exports = mongoose.model("request",requestSchema);