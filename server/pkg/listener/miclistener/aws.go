package miclistener

import (
	"bytes"
	"crypto/hmac"
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/cognitoidentity"
)

// Below follows six functions to handle AWS Signature V4 manually since
// AWS SDK's doesn't handle the signing correctly for the "iotdevicegateway"-service.
// References:
// 		https://github.com/aws/aws-sdk-go/issues/820
// 		https://github.com/aws/aws-sdk-go/issues/706
func micLogin() (*cognitoidentity.GetCredentialsForIdentityOutput, error) {
	reqBody, err := json.Marshal(map[string]string{
		"userName": micUsername,
		"password": micPassword,
	})
	if err != nil {
		return nil, err
	}

	client := http.Client{}
	request, err := http.NewRequest("POST", awsAPIGateway+"/auth/login", bytes.NewBuffer(reqBody))
	request.Header.Set("Content-type", "application/json")
	request.Header.Set("x-api-key", awsAPIKey)
	if err != nil {
		return nil, err
	}

	resp, err := client.Do(request)
	if err != nil {
		return nil, err
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var data map[string]credentials
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		return nil, err
	}

	token := data["credentials"].Token
	identityID := data["credentials"].IdentityID

	ses, _ := session.NewSession(&aws.Config{Region: aws.String(awsRegion)})
	svc := cognitoidentity.New(ses)
	credRes, err := svc.GetCredentialsForIdentity(&cognitoidentity.GetCredentialsForIdentityInput{
		IdentityId: aws.String(identityID),
		Logins: map[string]*string{
			"cognito-idp." + awsRegion + ".amazonaws.com/" + awsUserPool: &token,
		},
	})

	return credRes, err
}

func awsIotWsURL(accessKey string, secretKey string, sessionToken string, region string, endpoint string) string {
	host := fmt.Sprintf("%s.iot.%s.amazonaws.com", endpoint, region)

	// According to docs, time must be within 5min of actual time (or at least according to AWS servers)
	now := time.Now().UTC()

	dateLong := now.Format("20060102T150405Z")
	dateShort := dateLong[:8]
	serviceName := "iotdevicegateway"
	scope := fmt.Sprintf("%s/%s/%s/aws4_request", dateShort, region, serviceName)
	alg := "AWS4-HMAC-SHA256"
	q := [][2]string{
		{"X-Amz-Algorithm", alg},
		{"X-Amz-Credential", accessKey + "/" + scope},
		{"X-Amz-Date", dateLong},
		{"X-Amz-SignedHeaders", "host"},
	}

	query := awsQueryParams(q)

	signKey := awsSignKey(secretKey, dateShort, region, serviceName)
	stringToSign := awsSignString(accessKey, secretKey, query, host, dateLong, alg, scope)
	signature := fmt.Sprintf("%x", awsHmac(signKey, []byte(stringToSign)))

	wsurl := fmt.Sprintf("wss://%s/mqtt?%s&X-Amz-Signature=%s", host, query, signature)

	if sessionToken != "" {
		wsurl = fmt.Sprintf("%s&X-Amz-Security-Token=%s", wsurl, url.QueryEscape(sessionToken))
	}

	return wsurl
}

func awsQueryParams(q [][2]string) string {
	var buff bytes.Buffer
	var i int
	for _, param := range q {
		if i != 0 {
			buff.WriteRune('&')
		}
		i++
		buff.WriteString(param[0])
		buff.WriteRune('=')
		buff.WriteString(url.QueryEscape(param[1]))
	}
	return buff.String()
}

func awsSignString(accessKey string, secretKey string, query string, host string, dateLongStr string, alg string, scopeStr string) string {
	emptyStringHash := "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
	req := strings.Join([]string{
		"GET",
		"/mqtt",
		query,
		"host:" + host,
		"", // separator
		"host",
		emptyStringHash,
	}, "\n")
	return strings.Join([]string{
		alg,
		dateLongStr,
		scopeStr,
		awsSha(req),
	}, "\n")
}

func awsHmac(key []byte, data []byte) []byte {
	h := hmac.New(sha256.New, key)
	h.Write(data)
	return h.Sum(nil)
}

func awsSignKey(secretKey string, dateShort string, region string, serviceName string) []byte {
	h := awsHmac([]byte("AWS4"+secretKey), []byte(dateShort))
	h = awsHmac(h, []byte(region))
	h = awsHmac(h, []byte(serviceName))
	h = awsHmac(h, []byte("aws4_request"))
	return h
}

func awsSha(in string) string {
	h := sha256.New()
	fmt.Fprintf(h, "%s", in)
	return fmt.Sprintf("%x", h.Sum(nil))
}
