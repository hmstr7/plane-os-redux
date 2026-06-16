from datetime import datetime

from pydantic import ValidationError
import pytest

from utils.data import Control, Message, MessageType

def test_Control_flags():
    good_control = Control(
        elevator=-0.7,
        l_aileron=0.1,
        r_aileron=0.1,
        l_thrust=0
    )
    assert good_control.f_asym_ailerons
    assert not good_control.f_diff_thrust

def test_Control_invalid():
    with pytest.raises(ValidationError):
        bad_control = Control(
            elevator=1.2,
            l_aileron="ab",  # pyright: ignore[reportArgumentType]
            l_thrust=0
        )
    assert True

def test_Message():
    good_message = Message(
        timestamp=datetime.now(),
        mtype=MessageType.CONTROL,
        payload=Control(
            elevator=-0.7,
            l_aileron=0.1,
            l_thrust=0
        )
    )
    assert True

def test_Message_invalid():
    with pytest.raises(ValidationError, match="Message payload doesn't match the message type."):
        bad_message = Message(
            timestamp=datetime.now(),
            mtype=MessageType.CONTROL,
            payload='foo'
        )