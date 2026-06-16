from typing import Annotated, Literal, TypeAliasType
from datetime import datetime
from typing_extensions import Self
from enum import Enum
from pydantic import BaseModel, ConfigDict, Field, ValidationError, computed_field, model_validator

class Control(BaseModel):
    model_config = ConfigDict()  # pyright: ignore[reportUnannotatedClassAttribute]

    elevator: Annotated[float, Field(ge=-1.,le=1.)]
    l_aileron: Annotated[float, Field(ge=-1.,le=1.)]
    l_thrust: Annotated[float, Field(ge=0.,le=1.)]
    r_aileron: Annotated[float, Field(ge=-1.,le=1.)] | None = None
    r_thrust: Annotated[float, Field(ge=0.,le=1.)] | None = None

    # auto flags
    @computed_field
    @property
    def f_diff_thrust(self) -> bool:
        return self.r_thrust != self.l_thrust
    @computed_field
    @property
    def f_asym_ailerons(self) -> bool:
        return self.r_aileron != -self.l_aileron

    # add the missing values
    @model_validator(mode='after')
    def validate_control(self) -> Self:
        if not self.r_aileron:
            self.r_aileron = -self.l_aileron
        if not self.r_thrust:
            self.r_thrust = self.l_thrust
        return self

class MessageType(str,Enum):
    CONTROL = "ctl"
    SIGNAL = "sig"

type Signal = Literal["foo"] | Literal["bar"]

class Message(BaseModel):
    timestamp: datetime
    mtype: MessageType
    payload: Control | Signal
    
    @model_validator(mode='after')
    def validate_message(self) -> Self:
        
        valid_table: dict[MessageType,type|TypeAliasType] = {
            MessageType.CONTROL: Control,
            MessageType.SIGNAL: Signal
        }
        
        print("Mtype: ", self.mtype)
        print("Payload type", type(self.payload))

        if type(self.payload) is not valid_table[self.mtype]:
            raise ValueError("Message payload doesn't match the message type.")

        return self
