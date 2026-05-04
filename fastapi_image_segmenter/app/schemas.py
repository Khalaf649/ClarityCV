from pydantic import BaseModel, Field
from typing import Dict, List, Optional, Any


class MethodInfo(BaseModel):
    name: str
    category: str
    accepts: str
    description: str
    params: Dict[str, Any]


class SegmentResponse(BaseModel):
    method: str
    filename: str
    mime_type: str = "image/png"
    image_base64: str
    metadata: Dict[str, Any] = Field(default_factory=dict)


class MethodsResponse(BaseModel):
    thresholding: List[MethodInfo]
    segmentation: List[MethodInfo]
