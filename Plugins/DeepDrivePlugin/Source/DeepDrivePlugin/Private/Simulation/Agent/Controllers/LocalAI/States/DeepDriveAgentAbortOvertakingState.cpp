
#include "DeepDrivePluginPrivatePCH.h"
#include "Private/Simulation/Agent/Controllers/LocalAI/States/DeepDriveAgentAbortOvertakingState.h"
#include "Public/Simulation/Agent/Controllers/DeepDriveAgentLocalAIController.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSpeedController.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSteeringController.h"
#include "Public/Simulation/Agent/Controllers/DeepDriveAgentLocalAIController.h"
#include "Public/Simulation/Agent/DeepDriveAgent.h"


DeepDriveAgentAbortOvertakingState::DeepDriveAgentAbortOvertakingState(DeepDriveAgentLocalAIStateMachine &stateMachine)
	: DeepDriveAgentLocalAIStateBase(stateMachine, "AbortOvertaking")
{

}


void DeepDriveAgentAbortOvertakingState::enter(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	float distanceToNextAgent = -1.0f;
	float distanceToPrevAgent = -1.0f;
	ADeepDriveAgent *nextAgent = ctx.agent.getNextAgent(ctx.configuration.GapBetweenAgents, &distanceToNextAgent);
	ADeepDriveAgent *prevAgent = ctx.agent.getPrevAgent(ctx.configuration.GapBetweenAgents, &distanceToPrevAgent);

	m_PullInTimeFactor = 1.0f / (0.5f * ctx.configuration.ChangeLaneDuration);
	m_PullInAlpha = 1.0f;

	UE_LOG(LogDeepDriveAgentLocalAIController, Log, TEXT("AbortOvertaking next %s %f prev %s %f"), *(nextAgent ? nextAgent->GetName() : FString("")), distanceToNextAgent, *(prevAgent ? prevAgent->GetName() : FString("")), distanceToPrevAgent );
}

void DeepDriveAgentAbortOvertakingState::update(DeepDriveAgentLocalAIStateMachineContext &ctx, float dT)
{
	m_PullInAlpha -= m_PullInTimeFactor * dT;
	m_curOffset = FMath::Lerp(0.0f, ctx.side_offset, m_PullInAlpha);

	if (m_PullInAlpha <= 0.0f)
	{
		m_StateMachine.setNextState("Cruise");
	}

	float desiredSpeed = ctx.local_ai_ctrl.getDesiredSpeed();
	desiredSpeed = ctx.speed_controller.limitSpeedByTrack(desiredSpeed, ctx.configuration.SpeedLimitFactor);

	float safetyDistance = ctx.local_ai_ctrl.calculateSafetyDistance();
	float curDistanceToNext = 0.0f;
	ADeepDriveAgent *nextAgent = ctx.agent.getNextAgent(2.0f * safetyDistance, &curDistanceToNext);
	ctx.speed_controller.update(dT, desiredSpeed, nextAgent ? safetyDistance : -1.0f, curDistanceToNext);

	ctx.steering_controller.update(dT, desiredSpeed, m_curOffset);
}

void DeepDriveAgentAbortOvertakingState::exit(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	ctx.wait_time_before_overtaking = 4.0f;
	ctx.side_offset = m_curOffset;
}
