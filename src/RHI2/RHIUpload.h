#pragma once

namespace RHI
{
    class UploadBatch
    {
    public:
        virtual ~UploadBatch() = default;

        virtual bool Begin() = 0;
        virtual bool SubmitAndWait() = 0;
    };
} // namespace RHI
