const jwt = require("jsonwebtoken");
const cookieParser = require("cookie-parser");
const config = process.env;

const verifyToken = (req, res, next) => {
	const token =
	req.body.token || req.query.token || req.headers["x-access-token"] || req.cookies.access_token;

	if (!token){
		return res.status(403).send("Authentication is required - Missing token");
	}
	try{
		const decoded = jwt.verify(token, config.TOKEN_KEY);
		req.user = decoded;
	} catch (err){
		return res.status(401).send("Invalid token.");
	}
	return next();
};

const verifyAdmin = (req, res, next) => {
	const admin_token =
	req.body.token || req.query.token || req.headers["x-access-token"] || req.cookies.access_token;
	if (!admin_token){
		return res.status(403).send("Authentication is required - Missing token");
	}
	try {
		const token_data = jwt.verify(admin_token, config.TOKEN_KEY);
		req.emp_id = token_data.emp_id;
		if (req.emp_id != 1){
			return res.status(403).send("Administrative authentication is required for admin console.")
		}
		return next();
	} catch {
		return res.status(403).send("something went wrong with admin validation");
	}
};
exports.verifyToken = verifyToken;
exports.verifyAdmin = verifyAdmin;