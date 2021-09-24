/**
 * Verify the GCP KMS implementation can handle a buggy KMS.
 */

load("jstests/client_encrypt/lib/mock_kms.js");
load('jstests/ssl/libs/ssl_helpers.js');

(function() {
"use strict";

const x509_options = {
    sslMode: "requireSSL",
    sslPEMKeyFile: SERVER_CERT,
    sslCAFile: CA_CERT
};

const mockKey = {
    projectId: "mock",
    location: "global",
    keyRing: "mock-key-ring",
    keyName: "mock-key"
};

const randomAlgorithm = "AEAD_AES_256_CBC_HMAC_SHA_512-Random";

const conn = MongoRunner.runMongod(x509_options);
const test = conn.getDB("test");
const collection = test.coll;

function runKMS(mock_kms, func) {
    mock_kms.start();

    const gcpKMS = {
        email: "access@mongodb.com",
        endpoint: mock_kms.getURL(),
        // Arbitrary private key generated by openssl encoded in base64. A correctly formatted key
        // is required so the shell doesn't reject it, but the mock server doesn't actually use the
        // key to encrypt/decrypt.
        // The key is generated with `openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048`
        // which fulfills Windows's stricter requirements for RSA private keys
        privateKey:
            "MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQDDF3EoKeYp4GKgVckkSxe9Hr81TX/GHW+vSkwxZpI0LOia" +
            "rffIbv1IZ8xBD+HEL16E60pPZDYyNsmvNe6HRacSukTBM1peO5DrbM0VglEudoKc7TIT6trKFb1srd6WDUoGLZ6Xgm2KCmqZesV9VnWe" +
            "aLmbTa3TAZVn8nfDoRdLVl2ecE8BxrH63niMLQZl8qhcmeu61RQkOHbupTTovgH3PkufCOqgB01QHEcKgvPQBWqQvjms66U6Nfko2GMi" +
            "V5lPWWsJVBfNFU96M63QRxmjPWRoNNhui4xGPcm8u9GXo0p+83Ct2NZ7o3SxpaVUQaC2a2vbjADSI9G95fxM9z/zAgMBAAECggEBALOu" +
            "/pi4ZnXBZfU4rcaQpy+XhxKH65xD9l6jdqO1TgliJ2Z3vpTLrNqoR1bRUuYHnu2bbFjM+qGrFn0aljPe8i9sgfDT5HKQODytfAJIgY7i" +
            "tg/k40+26oZgGZRkW3MmkDw6fiwbg9o1F9N+YTC8lh4tZG3m0KdceQhBKQ90amkGaWsunC5/ZyI7Ip8JFFmkmoZHpiKJPYHI3dMNQyJK" +
            "cExSkw050UILIblPeA0AeDrtSBrz/5KfPKy0Wsh3cpuM9lE4dqupCnvp/GUZcSoG4NLg+I1FHQ/FkvY8AXtadES0JAty3E9GC60IWwvg" +
            "tdajFo4e1wlfyLgCFhAdsxslBokCgYEA7Emng/zBBF6IecuZWlekM286sUhLE2fSFRg+i5CjqsIJ0yAAIKAy5Qs8PV0TXtgLEg7M0nxG" +
            "9mnstkN5M2FSD4qEPe/teOJSzxgRavvqQCMtB9ABtz3nsXDQZbr5+/0bUxOsDbYW22t8dCavZ1u1SG+eC5PH/AGXLATBqR8Lfy8CgYEA" +
            "0134dkbZzy07z5cHDzTmsBZnhJt8W+Hix+5IViSnw3W+OdJFjMmUW8OmmBoMqzl69EaIU44bNpcLE1UOqNa74OGdNMid3p6JLUPSsiuh" +
            "9J1tH03+ZvubJQZiDiXDyXO7OeAX3ZQv0GAnK+LpYClcI50DwE655zzdbIn39iiuun0CgYEAn9XDBzl2p6n6z8i117LpVBGttjac8meM" +
            "aNCZnncc/2l6k+JVs7wqMV3ERg4sCEBEXNa+HrQKnK1Sfoht+B+hDvo4Ml2WWete8M/rGF+IOhKRZ3OBdZ7el90kW2x7pcW1MiFghXXj" +
            "SFIRQdDZXiVfH7zBQDubUBETXadqCSkC8ekCgYA6F22eNELQqgHyP/P0vflZFA9HZuR67E5D3L2Mz248TjQF+ECdPRnFTrSOwToSJS4h" +
            "zPDS5g+cpU6p9Yqd5MamO9vVEf4xnSjeg/F4fn14mXvQSsNM0oIFXwe8E60HxQMEGQ72GzA4+PRLH4Y8o6FrOFA7nmeBojzJA/JeeTfs" +
            "kQKBgQCh2Y1oIk7VcaP8F5vg1tw8VUuPadU3WqtDscenAli5Syp3ngDoIjtVII6mM8DxIE9tOrI+F653T/xcuVXjnPIymZh5LQttNABW" +
            "ZE1RXp16C1uFsg3F9U1wIljep+F6D/mjySkoaM8PwE1miLwThPGvgt2YSCyuig1OMOzOXlIUnQ==",
    };

    const clientSideFLEOptions = {
        kmsProviders: {
            gcp: gcpKMS,
        },
        keyVaultNamespace: "test.coll",
        schemaMap: {},
    };

    const shell = Mongo(conn.host, clientSideFLEOptions);
    const cleanCacheShell = Mongo(conn.host, clientSideFLEOptions);

    collection.drop();

    func(shell, cleanCacheShell);

    mock_kms.stop();
}

// OAuth faults must be tested first so a cached token can't be used
function testBadOAuthRequestResult() {
    const mock_kms = new MockKMSServerGCP(FAULT_OAUTH, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();

        const error = assert.throws(() => keyVault.createKey("gcp", mockKey, ["mongoKey"]));
        assert.eq(
            error,
            "Error: code 9: FailedToParse: Expecting '{': offset:0 of:Internal Error of some sort.");
    });
}

testBadOAuthRequestResult();

function testBadOAuthRequestError() {
    const mock_kms = new MockKMSServerGCP(FAULT_OAUTH_CORRECT_FORMAT, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();

        const error = assert.throws(() => keyVault.createKey("gcp", mockKey, ["mongoKey"]));
        assert.commandFailedWithCode(error, [ErrorCodes.OperationFailed]);
        assert.eq(
            error,
            "Error: Failed to make oauth request: GCP OAuth Error : FAULT_OAUTH_CORRECT_FORMAT");
    });
}

testBadOAuthRequestError();

function testBadEncryptResult() {
    const mock_kms = new MockKMSServerGCP(FAULT_ENCRYPT, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();

        assert.throws(() => keyVault.createKey("gcp", mockKey, ["mongoKey"]));
        assert.eq(keyVault.getKeys("mongoKey").toArray().length, 0);
    });
}

testBadEncryptResult();

function testBadEncryptError() {
    const mock_kms = new MockKMSServerGCP(FAULT_ENCRYPT_CORRECT_FORMAT, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();
        let error = assert.throws(() => keyVault.createKey("gcp", mockKey, ["mongoKey"]));
        assert.commandFailedWithCode(error, [5256006]);
    });
}

testBadEncryptError();

function testBadDecryptResult() {
    const mock_kms = new MockKMSServerGCP(FAULT_DECRYPT, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();
        const keyId = keyVault.createKey("gcp", mockKey, ["mongoKey"]);
        const str = "mongo";
        assert.throws(() => {
            const encStr = shell.getClientEncryption().encrypt(keyId, str, randomAlgorithm);
        });
    });
}

testBadDecryptResult();

function testBadDecryptKeyResult() {
    const mock_kms = new MockKMSServerGCP(FAULT_DECRYPT_WRONG_KEY, true);

    runKMS(mock_kms, (shell, cleanCacheShell) => {
        const keyVault = shell.getKeyVault();

        keyVault.createKey("gcp", mockKey, ["mongoKey"]);
        const keyId = keyVault.getKeys("mongoKey").toArray()[0]._id;
        const str = "mongo";
        const encStr = shell.getClientEncryption().encrypt(keyId, str, randomAlgorithm);

        mock_kms.enableFaults();

        assert.throws(() => {
            let str = cleanCacheShell.decrypt(encStr);
        });
    });
}

testBadDecryptKeyResult();

function testBadDecryptError() {
    const mock_kms = new MockKMSServerGCP(FAULT_DECRYPT_CORRECT_FORMAT, false);

    runKMS(mock_kms, (shell) => {
        const keyVault = shell.getKeyVault();

        keyVault.createKey("gcp", mockKey, ["mongoKey"]);
        const keyId = keyVault.getKeys("mongoKey").toArray()[0]._id;
        const str = "mongo";
        let error = assert.throws(() => {
            const encStr = shell.getClientEncryption().encrypt(keyId, str, randomAlgorithm);
        });
        assert.commandFailedWithCode(error, [5256008]);
    });
}

testBadDecryptError();

MongoRunner.stopMongod(conn);
})();
