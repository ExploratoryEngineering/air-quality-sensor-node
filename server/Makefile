ifeq ($(GOPATH),)
GOPATH := $(HOME)/go
endif

all: test lint vet build

clean:
	@find . -name "*-wal" -delete
	@find . -name "*-shm" -delete
	@rm -f bin/*.linux

build: aq

aq:
	@cd cmd/aq && go build -o ../../bin/aq

check: lint vet staticcheck revive

lint:
	@golint ./...

vet:
	@go vet ./...

staticcheck:
	@staticcheck ./...

revive:
	@revive ./...

test:
	@go test ./...

test_verbose:
	@go test ./... -v

test_race:
	@go test ./... -race

test_all: test_cover test_race

benchmark:
	cd output && go test -bench .

gen:
	@go generate ./...


# Here be dragons
dep_install:
	(cd $(GOPATH); go get -u github.com/golang/protobuf/protoc-gen-go)
	(cd $(GOPATH); go get -u github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway)
	(cd $(GOPATH); go get -u github.com/grpc-ecosystem/grpc-gateway/protoc-gen-swagger)
	(cd $(GOPATH); go get -u github.com/golang/protobuf/protoc-gen-go)
	(cd $(GOPATH); go get -u github.com/favadi/protoc-go-inject-tag)
