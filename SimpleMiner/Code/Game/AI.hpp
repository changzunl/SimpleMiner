#include "Controller.hpp"
#include "Engine/Core/Stopwatch.hpp"

class AI : public Controller
{
public:
	AI();
	virtual ~AI();

	virtual void Update( float deltaSeconds ) override;

	Stopwatch m_meleeStopwatch;
	Stopwatch m_updateStopwatch;
};

