const {onCall} = require("firebase-functions/v2/https");
const logger = require("firebase-functions/logger");

exports.foo = onCall((data, context) => {
    //logger.info("foo: data", data);
    //logger.info("foo: context", context);
    return 200;
});
