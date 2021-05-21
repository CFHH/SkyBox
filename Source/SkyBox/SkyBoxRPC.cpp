#include "SkyBoxRPC.h"
#include "CoreMinimal.h"


void SkyBoxServiceImpl::RunServer()
{
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！SkyBoxServiceImpl::RunServer()"));
    std::string server_address("0.0.0.0:50051");
    SkyBoxServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！RPC Server listening on %s"), server_address.c_str());
    server->Wait();
}

void SkyBoxServiceImpl::ShutDownServer()
{
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！SkyBoxServiceImpl::ShutDownServer()"));
    //grpc::Server::Shutdown();
    //grpc::CompletionQueue::Shutdown();
}

grpc::Status SkyBoxServiceImpl::SayHello(grpc::ServerContext* context, const skybox::HelloRequest* request, skybox::HelloReply* reply)
{
    UE_LOG(LogTemp, Warning, TEXT("！！！！！！！！！！SkyBoxServiceImpl::SayHello()"));
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return grpc::Status::OK;
}

grpc::Status SkyBoxServiceImpl::GenerateSkyBox(grpc::ServerContext* context, const skybox::GenerateSkyBoxRequest* request, skybox::GenerateSkyBoxReply* reply)
{
    reply->set_job_id(-1);
    return grpc::Status::OK;
}


grpc::Status SkyBoxServiceImpl::QueryJob(grpc::ServerContext* context, const skybox::QueryJobRequest* request, skybox::QueryJobReply* reply)
{
    reply->set_job_id(-1);
    return grpc::Status::OK;
}
